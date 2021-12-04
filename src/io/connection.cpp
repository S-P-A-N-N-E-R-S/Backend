#include <networking/io/connection.hpp>

#include <algorithm>
#include <boost/asio/completion_condition.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <iostream>
#include <iterator>
#include <string>
#include <thread>

#include <handling/handler_utilities.hpp>
#include <networking/io/connection_handler.hpp>
#include <networking/requests/request_factory.hpp>
#include <networking/responses/new_job_response.hpp>
#include <networking/responses/response_factory.hpp>
#include <networking/responses/status_response.hpp>

#include <scheduler/scheduler.hpp>

#include <result.pb.h>

using boost::asio::async_read;
using boost::asio::buffer;
using boost::asio::transfer_exactly;
using boost::asio::ip::tcp;
using boost::system::error_code;
using ssl_socket = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

namespace server {

namespace {
    constexpr int LENGTH_FIELD_SIZE = 8;
}

connection::connection(size_t id, connection_handler &handler, ssl_socket sock)
    : m_identifier{id}
    , m_handler{handler}
    , m_sock{std::move(sock)}
{
    error_code error;
    m_sock.handshake(boost::asio::ssl::stream_base::server, error);
    if (error)
    {
        throw std::runtime_error{"Failed to perform handshake."};
    }
}

// connection::~connection()
// {
//     m_sock.close();
// }

void connection::handle()
{
    boost::asio::spawn(m_sock.get_executor(), [this](boost::asio::yield_context yield) {
        namespace io = boost::iostreams;

        std::vector<char> recv_buffer;
        size_t recv_size;

        // Read size of meta message
        if (!direct_read(yield, reinterpret_cast<char *const>(&recv_size), LENGTH_FIELD_SIZE))
        {
            respond_error(yield, graphs::ResponseContainer::READ_ERROR);
            return;
        }
        boost::endian::big_to_native_inplace(recv_size);
        recv_buffer.resize(recv_size);

        // Read and parse meta message
        if (!direct_read(yield, recv_buffer.data(), recv_buffer.size()))
        {
            respond_error(yield, graphs::ResponseContainer::READ_ERROR);
            return;
        }
        graphs::MetaData meta_proto;
        if (!meta_proto.ParseFromArray(recv_buffer.data(), recv_buffer.size()))
        {
            std::cout << "[CONNECTION] Protobuf parsing error: Meta Message\n";
            respond_error(yield, graphs::ResponseContainer::PROTO_PARSING_ERROR);
            return;
        }

        // Get message container size from meta message and read the container
        recv_buffer.resize(meta_proto.containersize());
        if (!direct_read(yield, recv_buffer.data(), recv_buffer.size()))
        {
            respond_error(yield, graphs::ResponseContainer::READ_ERROR);
            return;
        }

        // TODO: remove hardcoded string with env variables
        std::string connection_string =
            "host=localhost port=5432 user=spanner_user dbname=spanner_db "
            "password=pwd connect_timeout=10";
        database_wrapper database(connection_string);

        // TODO: change user_id from hardcoded to dynamically read from request
        const int user_id = 1;

        const auto type = meta_proto.type();

        // Process "non-job" requests immediately
        if (type == graphs::RequestType::AVAILABLE_HANDLERS)
        {
            respond(yield, meta_data{graphs::RequestType::AVAILABLE_HANDLERS},
                    response_factory::build_response(available_handlers()));
            return;
        }
        else if (type == graphs::RequestType::STATUS)
        {
            std::vector<job_entry> jobs = database.get_job_entries(user_id);

            graphs::StatusResponse status_response;
            auto respStates = status_response.mutable_states();

            for (const auto &job : jobs)
            {
                respStates->Add(database.get_status_data(job.job_id, user_id));
            }

            auto response = std::make_unique<server::status_response>(std::move(status_response),
                                                                      status_code::OK);

            respond(yield, meta_data{graphs::RequestType::STATUS},
                    response_factory::build_response(std::move(response)));
            return;
        }

        // Possibly long-running requests are scheduled by scheduler later on

        // TODO Check if we need to decompress the received data first -> meta message
        // Gzip decompression of container
        io::filtering_streambuf<io::input> in_str_buf;
        in_str_buf.push(io::gzip_decompressor{});
        in_str_buf.push(io::array_source{recv_buffer.data(), recv_buffer.size()});

        // Copy compressed data into the decompressed buffer
        std::vector<char> decompressed;
        decompressed.reserve(recv_buffer.size());
        std::copy(std::istreambuf_iterator<char>{&in_str_buf}, {},
                  std::back_inserter(decompressed));
        binary_data_view binary(reinterpret_cast<std::byte *>(decompressed.data()),
                                decompressed.size());

        if (type == graphs::RequestType::RESULT)
        {
            auto request_container = graphs::RequestContainer();
            if (!request_container.ParseFromArray(binary.data(), binary.size()))
            {
                std::cout << "[CONNECTION] Protobuf parsing error: RequestContainer\n";
                respond_error(yield, graphs::ResponseContainer::PROTO_PARSING_ERROR);
                return;
            }

            graphs::ResultRequest res_req;
            if (const bool ok = request_container.request().UnpackTo(&res_req); !ok)
            {
                std::cout << "[CONNECTION] Protobuf unpack error: ResultRequest\n";
                respond_error(yield, graphs::ResponseContainer::INVALID_REQUEST_ERROR);
                return;
            }

            const int job_id = res_req.jobid();

            meta_data job_meta_data = database.get_meta_data(job_id, user_id);
            auto [type, binary_response] = database.get_response_data_raw(job_id, user_id);

            // Add latest status information to the algorithm response
            graphs::ResponseContainer status_container;
            *(status_container.mutable_statusdata()) = database.get_status_data(job_id, user_id);
            size_t old_len = binary_response.size();
            binary_response.resize(old_len + status_container.ByteSizeLong());
            status_container.SerializeToArray(binary_response.data() + old_len,
                                              binary_response.size());

            respond(yield, job_meta_data, binary_response);
            return;
        }

        // TODO: Try-Catch to catch database or authentification errors
        int job_id = database.add_job(
            user_id, meta_data{type, meta_proto.handlertype(), meta_proto.jobname()}, binary);

        graphs::NewJobResponse new_job_resp;
        new_job_resp.set_jobid(job_id);

        auto response =
            std::make_unique<new_job_response>(std::move(new_job_resp), status_code::OK);

        respond(yield, meta_data{graphs::RequestType::NEW_JOB_RESPONSE},
                response_factory::build_response(std::move(response)));
    });
}

void connection::respond(boost::asio::yield_context &yield, const meta_data &meta_info,
                         const graphs::ResponseContainer &container)
{
    namespace io = boost::iostreams;

    graphs::MetaData meta_proto;
    meta_proto.set_type(meta_info.request_type);
    meta_proto.set_handlertype(meta_info.handler_type);
    meta_proto.set_jobname(meta_info.job_name);
    std::vector<char> container_data;

    {
        // Generate uncompressed buffer from protobuf container
        std::vector<char> uncompressed;
        uncompressed.resize(container.ByteSizeLong());
        container.SerializeToArray(uncompressed.data(), uncompressed.size());

        // Boost gzip compression
        io::filtering_streambuf<io::input> in_str_buf;
        in_str_buf.push(io::gzip_compressor{});
        in_str_buf.push(io::array_source{uncompressed.data(), uncompressed.size()});

        // Copy compressed data into the compressed buffer
        std::copy(std::istreambuf_iterator<char>{&in_str_buf}, {},
                  std::back_inserter(container_data));
        meta_proto.set_containersize(container_data.size());
    }

    uint64_t len = boost::endian::native_to_big(meta_proto.ByteSizeLong());

    std::vector<char> meta_data;
    meta_data.resize(meta_proto.GetCachedSize());
    meta_proto.SerializeToArray(meta_data.data(), meta_data.size());

    // Send data to client
    direct_write(yield, reinterpret_cast<const char *>(&len), LENGTH_FIELD_SIZE);
    direct_write(yield, meta_data.data(), meta_data.size());
    direct_write(yield, container_data.data(), container_data.size());

    // Connection finished
    m_handler.remove(m_identifier);
}

void connection::respond(boost::asio::yield_context &yield, const meta_data &meta_info,
                         binary_data_view binary)
{
    namespace io = boost::iostreams;

    graphs::MetaData meta_proto;
    meta_proto.set_type(meta_info.request_type);
    meta_proto.set_handlertype(meta_info.handler_type);
    meta_proto.set_jobname(meta_info.job_name);
    std::vector<char> container_data;

    {
        std::vector<char> uncompressed(
            reinterpret_cast<const char *>(binary.data()),
            reinterpret_cast<const char *>(binary.data() + binary.size()));

        // Boost gzip compression
        io::filtering_streambuf<io::input> in_str_buf;
        in_str_buf.push(io::gzip_compressor{});
        in_str_buf.push(io::array_source{uncompressed.data(), uncompressed.size()});

        // Copy compressed data into the compressed buffer
        std::copy(std::istreambuf_iterator<char>{&in_str_buf}, {},
                  std::back_inserter(container_data));
        meta_proto.set_containersize(container_data.size());
    }

    uint64_t len = boost::endian::native_to_big(meta_proto.ByteSizeLong());

    std::vector<char> meta_data;
    meta_data.resize(meta_proto.GetCachedSize());
    meta_proto.SerializeToArray(meta_data.data(), meta_data.size());

    // Send data to client
    direct_write(yield, reinterpret_cast<const char *>(&len), LENGTH_FIELD_SIZE);
    direct_write(yield, meta_data.data(), meta_data.size());
    direct_write(yield, container_data.data(), container_data.size());

    // Connection finished
    m_handler.remove(m_identifier);
}

void connection::respond_error(boost::asio::yield_context &yield,
                               graphs::ResponseContainer_StatusCode code)
{
    graphs::ResponseContainer response;
    response.set_status(code);
    respond(yield, meta_data{graphs::RequestType::UNDEFINED_REQUEST}, response);
}

bool connection::direct_read(boost::asio::yield_context &yield, char *const data, size_t length)
{
    error_code error;
    async_read(m_sock, buffer(data, length), transfer_exactly(length), yield[error]);
    if (error)
        std::cout << "[CONNECTION] Read error: " << error << '\n';
    return !error;
}

void connection::direct_write(boost::asio::yield_context &yield, const char *data, size_t length)
{
    do
    {
        error_code error;
        size_t bytes_sent = async_write(m_sock, buffer(data, length), yield[error]);
        if (!error)
        {
            data += bytes_sent;
            length -= bytes_sent;

            std::cout << "[CONNECTION] Sent " << bytes_sent << " bytes" << std::endl;
        }
    } while (length > 0);
}

}  // namespace server
