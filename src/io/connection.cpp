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

#include <handling/handler_proxy.hpp>
#include <networking/io/connection_handler.hpp>
#include <networking/requests/request_factory.hpp>
#include <networking/responses/response_factory.hpp>

#include <scheduler/scheduler.hpp>

using boost::asio::async_read;
using boost::asio::buffer;
using boost::asio::transfer_exactly;
using boost::asio::ip::tcp;
using boost::system::error_code;

namespace server {

namespace {
    constexpr int LENGTH_FIELD_SIZE = 8;
}

connection::connection(size_t id, connection_handler &handler, tcp::socket sock)
    : m_identifier{id}
    , m_handler{handler}
    , m_sock{std::move(sock)}
{
}

connection::~connection()
{
    m_sock.close();
}

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
        graphs::MetaData meta_data;
        if (!meta_data.ParseFromArray(recv_buffer.data(), recv_buffer.size()))
        {
            std::cout << "[CONNECTION] Protobuf parsing error: Meta Message\n";
            respond_error(yield, graphs::ResponseContainer::PROTO_PARSING_ERROR);
            return;
        }

        // Get message container size from meta message and read the container
        recv_buffer.resize(meta_data.containersize());
        if (!direct_read(yield, recv_buffer.data(), recv_buffer.size()))
        {
            respond_error(yield, graphs::ResponseContainer::READ_ERROR);
            return;
        }

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

        // Parse decompressed container
        graphs::RequestContainer request_container;
        if (!request_container.ParseFromArray(decompressed.data(), decompressed.size()))
        {
            std::cout << "[CONNECTION] Protobuf parsing error: Container\n";
            respond_error(yield, graphs::ResponseContainer::PROTO_PARSING_ERROR);
            return;
        }

        std::unique_ptr<server::abstract_response> response;
        // TODO: find a better way instead of manually checking for these "meta" handlers
        if (request_container.type() == graphs::RequestType::AVAILABLE_HANDLERS)
        {
            response = handler_proxy().available_handlers();
        }
        else
        {
            //This is a very quick and dirty and unsave fix and I hate it so much.
            // We absolutely should change this after RequestType was moved into MetaMessage
            binary_data binary;
            binary.resize(decompressed.size());
            for (size_t i = 0; i < decompressed.size(); i++)
            {
                binary[i] = static_cast<std::byte>(decompressed[i]);
            }

            // TODO: remove hardcoded string with env variables
            std::string connection_string =
                "host=localhost port=5432 user=spanner_user dbname=spanner_db "
                "password=pwd connect_timeout=10";
            database_wrapper database(connection_string);
            // TODO: Try-Catch to catch database or authentification errors
            int job_id = database.add_job(0, binary);
            response = std::unique_ptr<abstract_response>(nullptr);
        }

        server::response_factory res_factory;
        respond(yield, res_factory.build_response(std::move(response)));
    });
}

void connection::respond(boost::asio::yield_context &yield,
                         const graphs::ResponseContainer &container)
{
    namespace io = boost::iostreams;

    graphs::MetaData meta;
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
        meta.set_containersize(container_data.size());
    }

    uint64_t len = boost::endian::native_to_big(meta.ByteSizeLong());

    std::vector<char> meta_data;
    meta_data.resize(meta.GetCachedSize());
    meta.SerializeToArray(meta_data.data(), meta_data.size());

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
    respond(yield, response);
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
        size_t bytes_sent = m_sock.async_send(buffer(data, length), yield[error]);
        if (!error)
        {
            data += bytes_sent;
            length -= bytes_sent;

            std::cout << "[CONNECTION] Sent " << bytes_sent << " bytes" << std::endl;
        }
    } while (length > 0);
}

}  // namespace server
