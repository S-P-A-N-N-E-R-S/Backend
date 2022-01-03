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
#include "argon2.h"

#include <handling/handler_utilities.hpp>
#include <networking/io/connection_handler.hpp>
#include <networking/requests/request_factory.hpp>
#include <networking/responses/new_job_response.hpp>
#include <networking/responses/response_factory.hpp>
#include <networking/responses/status_response.hpp>
#include <persistence/database_wrapper.hpp>
#include <persistence/user.hpp>

#include <scheduler/scheduler.hpp>

#include <result.pb.h>

using boost::asio::async_read;
using boost::asio::buffer;
using boost::asio::transfer_exactly;
using boost::asio::ip::tcp;
using boost::asio::ssl::stream;
using boost::system::error_code;

namespace server {

namespace {
    constexpr int LENGTH_FIELD_SIZE = 8;

    bool check_password(const std::string &password, const server::binary_data &salt,
                        const server::binary_data &correct_hash)
    {
        static constexpr const size_t HASH_LENGTH = 32;
        static constexpr const uint32_t PASSES = 2;
        static constexpr const uint32_t MEM_USE = (1 << 16);  // 64 MB
        static constexpr const uint32_t THREADS = 1;

        server::binary_data cur_hash(HASH_LENGTH, std::byte{0});

        if (const int result =
                argon2id_hash_raw(PASSES, MEM_USE, THREADS, password.c_str(), password.length(),
                                  salt.data(), salt.length(), cur_hash.data(), cur_hash.length());
            result != ARGON2_OK)
        {
            // TODO: What to do if result != ARGON2_OK? At least log this somewhere.
            return false;
        }

        return (cur_hash == correct_hash);
    }
}  // namespace

connection::connection(size_t id, connection_handler &handler, socket_ptr sock)
    : m_identifier{id}
    , m_handler{handler}
    , m_sock{std::move(sock)}
{
#ifndef SPANNERS_UNENCRYPTED_CONNECTION
    error_code error;
    m_sock->handshake(boost::asio::ssl::stream_base::server, error);
    if (error)
    {
        throw std::runtime_error{"Failed to perform handshake"};
    }
#endif
}

void connection::handle()
{
    boost::asio::spawn(m_sock->get_executor(), [this](boost::asio::yield_context yield) {
        try
        {
            handle_internal(yield);
        }
        catch (graphs::ResponseContainer::StatusCode &error)
        {
            std::cout << "[ERROR] " << error << '\n';
            respond_error(yield, error);
            return;
        }
        catch (graphs::ErrorType &error)
        {
            std::cout << "[ERROR] " << error << '\n';
            respond_error(yield, error);
            return;
        }
        catch (std::exception &ex)
        {
            std::cout << "[ERROR]" << ex.what() << '\n';
            respond_error(yield, graphs::ResponseContainer::ERROR);
            return;
        }
    });
}

void connection::handle_internal(boost::asio::yield_context &yield)
{
    // Read size of meta message
    size_t recv_size;
    if (!direct_read(yield, reinterpret_cast<char *const>(&recv_size), LENGTH_FIELD_SIZE))
    {
        throw graphs::ResponseContainer::READ_ERROR;
    }
    boost::endian::big_to_native_inplace(recv_size);

    // Read and parse meta message
    graphs::MetaData meta_proto = read_message<graphs::MetaData>(yield, recv_size);

    // TODO: remove hardcoded string with env variables
    // User authentication
    std::string connection_string = "host=localhost port=5432 user=spanner_user dbname=spanner_db "
                                    "password=pwd connect_timeout=10";
    database_wrapper database(connection_string);

    const auto user = database.get_user(meta_proto.user().name());
    if (!user)
    {
        // TODO: Log this incident
        throw graphs::ErrorType::UNAUTHORIZED;
    }

    if (!check_password(meta_proto.user().password(), user->salt, user->pw_hash))
    {
        // TODO: Log this incident
        throw graphs::ErrorType::UNAUTHORIZED;
    }

    // Reuquest handling
    const auto type = meta_proto.type();
    switch (type)
    {
        case graphs::RequestType::AVAILABLE_HANDLERS: {
            handle_available_handlers(yield);
            break;
        }
        case graphs::RequestType::STATUS: {
            handle_status(yield, database, *user);
            break;
        }
        case graphs::RequestType::RESULT: {
            handle_result(yield, database, meta_proto, *user);
            break;
        }
        default: {
            handle_new_job(yield, database, meta_proto, *user);
            break;
        }
    }
}

void connection::handle_available_handlers(boost::asio::yield_context &yield)
{
    respond(yield, meta_data{graphs::RequestType::AVAILABLE_HANDLERS},
            response_factory::build_response(available_handlers()));
}

void connection::handle_status(boost::asio::yield_context &yield, database_wrapper &db,
                               const user &user)
{
    std::vector<job_entry> jobs = db.get_job_entries(user.user_id);

    graphs::StatusResponse status_response;
    auto respStates = status_response.mutable_states();

    for (const auto &job : jobs)
    {
        respStates->Add(db.get_status_data(job.job_id, user.user_id));
    }

    auto response =
        std::make_unique<server::status_response>(std::move(status_response), status_code::OK);

    respond(yield, meta_data{graphs::RequestType::STATUS},
            response_factory::build_response(std::move(response)));
}

void connection::handle_result(boost::asio::yield_context &yield, database_wrapper &db,
                               const graphs::MetaData &meta, const user &user)
{
    graphs::RequestContainer request_container =
        read_message<graphs::RequestContainer>(yield, meta.containersize());

    graphs::ResultRequest res_req;
    if (const bool ok = request_container.request().UnpackTo(&res_req); !ok)
    {
        throw graphs::ResponseContainer::INVALID_REQUEST_ERROR;
    }

    const int job_id = res_req.jobid();

    meta_data job_meta_data = db.get_meta_data(job_id, user.user_id);
    auto [type, binary_response] = db.get_response_data_raw(job_id, user.user_id);

    // Add latest status information to the algorithm response
    graphs::ResponseContainer status_container;
    *(status_container.mutable_statusdata()) = db.get_status_data(job_id, user.user_id);
    size_t old_len = binary_response.size();
    binary_response.resize(old_len + status_container.ByteSizeLong());
    status_container.SerializeToArray(binary_response.data() + old_len, binary_response.size());

    respond(yield, job_meta_data, binary_response);
}

void connection::handle_new_job(boost::asio::yield_context &yield, database_wrapper &db,
                                const graphs::MetaData &meta, const user &user)
{
    namespace io = boost::iostreams;

    std::vector<char> recv_buffer;
    recv_buffer.resize(meta.containersize());
    if (!direct_read(yield, recv_buffer.data(), recv_buffer.size()))
    {
        throw graphs::ResponseContainer::READ_ERROR;
    }

    io::filtering_streambuf<io::input> in_str_buf;
    in_str_buf.push(io::gzip_decompressor{});
    in_str_buf.push(io::array_source{recv_buffer.data(), recv_buffer.size()});

    // Copy compressed data into the decompressed buffer
    std::vector<char> decompressed;
    decompressed.reserve(recv_buffer.size());
    decompressed.assign(std::istreambuf_iterator<char>{&in_str_buf}, {});
    binary_data_view binary(reinterpret_cast<std::byte *>(decompressed.data()),
                            decompressed.size());

    int job_id = db.add_job(user.user_id,
                            meta_data{meta.type(), meta.handlertype(), meta.jobname()}, binary);

    graphs::NewJobResponse new_job_resp;
    new_job_resp.set_jobid(job_id);

    auto response = std::make_unique<new_job_response>(std::move(new_job_resp), status_code::OK);

    respond(yield, meta_data{graphs::RequestType::NEW_JOB_RESPONSE},
            response_factory::build_response(std::move(response)));
}

template <typename MESSAGE_TYPE>
MESSAGE_TYPE connection::read_message(boost::asio::yield_context &yield, size_t len)
{
    std::vector<char> recv_buffer;
    try
    {
        recv_buffer.resize(len);
    }
    catch (std::bad_alloc &)
    {
        throw graphs::ResponseContainer::READ_ERROR;
    }

    if (!direct_read(yield, recv_buffer.data(), recv_buffer.size()))
    {
        throw graphs::ResponseContainer::READ_ERROR;
    }

    if constexpr (!std::is_same_v<MESSAGE_TYPE, graphs::MetaData>)
    {
        // Other messages are compressed using GZIP compression
        namespace io = boost::iostreams;

        io::filtering_streambuf<io::input> in_str_buf;
        in_str_buf.push(io::gzip_decompressor{});
        in_str_buf.push(io::array_source{recv_buffer.data(), recv_buffer.size()});
        recv_buffer.assign(std::istreambuf_iterator<char>{&in_str_buf}, {});
    }

    MESSAGE_TYPE msg;
    if (!msg.ParseFromArray(recv_buffer.data(), recv_buffer.size()))
    {
        throw graphs::ResponseContainer::PROTO_PARSING_ERROR;
    }
    return msg;
}

template <class Serializable>
void connection::respond(boost::asio::yield_context &yield, const meta_data &meta_info,
                         const Serializable &message)
{
    namespace io = boost::iostreams;

    graphs::MetaData meta_proto;
    meta_proto.set_type(meta_info.request_type);
    meta_proto.set_handlertype(meta_info.handler_type);
    meta_proto.set_jobname(meta_info.job_name);
    std::vector<char> message_data;

    {
        // Generate uncompressed buffer from protobuf message
        std::vector<char> uncompressed;
        uncompressed.resize(message.ByteSizeLong());
        message.SerializeToArray(uncompressed.data(), uncompressed.size());

        // Boost gzip compression
        io::filtering_streambuf<io::input> in_str_buf;
        in_str_buf.push(io::gzip_compressor{});
        in_str_buf.push(io::array_source{uncompressed.data(), uncompressed.size()});

        // Copy compressed data into the compressed buffer
        std::copy(std::istreambuf_iterator<char>{&in_str_buf}, {},
                  std::back_inserter(message_data));
        meta_proto.set_containersize(message_data.size());
    }

    uint64_t len = boost::endian::native_to_big(meta_proto.ByteSizeLong());

    std::vector<char> meta_data;
    meta_data.resize(meta_proto.GetCachedSize());
    meta_proto.SerializeToArray(meta_data.data(), meta_data.size());

    // Send data to client
    direct_write(yield, reinterpret_cast<const char *>(&len), LENGTH_FIELD_SIZE);
    direct_write(yield, meta_data.data(), meta_data.size());
    direct_write(yield, message_data.data(), message_data.size());

    // Connection finished
    m_handler.remove(m_identifier);
}

void connection::respond(boost::asio::yield_context &yield, const meta_data &meta_info,
                         const binary_data &binary)
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
    respond(yield, meta_data{graphs::RequestType::ERROR}, response);
}

void connection::respond_error(boost::asio::yield_context &yield, graphs::ErrorType error_type)
{
    graphs::ErrorMessage msg;
    msg.set_type(error_type);
    respond(yield, meta_data{graphs::RequestType::ERROR}, msg);
}

bool connection::direct_read(boost::asio::yield_context &yield, char *const data, size_t length)
{
    error_code error;
    async_read(*m_sock, buffer(data, length), transfer_exactly(length), yield[error]);
    if (error)
        std::cout << "[CONNECTION] Read error: " << error << '\n';
    return !error;
}

void connection::direct_write(boost::asio::yield_context &yield, const char *data, size_t length)
{
    do
    {
        error_code error;
        size_t bytes_sent = async_write(*m_sock, buffer(data, length), yield[error]);
        if (!error)
        {
            data += bytes_sent;
            length -= bytes_sent;

            std::cout << "[CONNECTION] Sent " << bytes_sent << " bytes" << std::endl;
        }
    } while (length > 0);
}

}  // namespace server
