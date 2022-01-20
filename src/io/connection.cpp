#include <networking/io/connection.hpp>

#include <boost/asio/completion_condition.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <iostream>
#include <iterator>
#include <string>
#include "argon2.h"

#include <config/config.hpp>
#include <handling/handler_utilities.hpp>
#include <networking/io/connection_handler.hpp>
#include <networking/io/request_handling.hpp>
#include <networking/requests/request_factory.hpp>
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
using graphs::ErrorMessage;
using graphs::ErrorType;
using graphs::MetaData;
using graphs::RequestContainer;
using graphs::RequestType;
using graphs::ResponseContainer;
using namespace server::request_handling;

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
        catch (ResponseContainer::StatusCode &error)
        {
            std::cout << "[ERROR] " << error << '\n';
            respond_error(yield, error);
            return;
        }
        catch (ErrorType &error)
        {
            std::cout << "[ERROR] " << error << '\n';
            respond_error(yield, error);
            return;
        }
        catch (std::exception &ex)
        {
            std::cout << "[ERROR]" << ex.what() << '\n';
            respond_error(yield, ResponseContainer::ERROR);
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
        throw ResponseContainer::READ_ERROR;
    }
    boost::endian::big_to_native_inplace(recv_size);

    // Read and parse meta message
    MetaData meta_proto = read_message<MetaData>(yield, recv_size);

    // User authentication
    std::string connection_string = server::get_db_connection_string();
    database_wrapper database(connection_string);

    const auto user = database.get_user(meta_proto.user().name());
    if (!user)
    {
        // TODO: Log this incident
        throw ErrorType::UNAUTHORIZED;
    }

    if (!check_password(meta_proto.user().password(), user->salt, user->pw_hash))
    {
        // TODO: Log this incident
        throw ErrorType::UNAUTHORIZED;
    }

    // Reuquest handling
    switch (meta_proto.type())
    {
        case RequestType::AVAILABLE_HANDLERS: {
            auto [response_meta, response] = handle_available_handlers();
            respond(yield, response_meta, response);
            break;
        }
        case RequestType::STATUS: {
            auto [response_meta, response] = handle_status(database, *user);
            respond(yield, response_meta, response);
            break;
        }
        case RequestType::RESULT: {
            RequestContainer request =
                read_message<RequestContainer>(yield, meta_proto.containersize());

            auto [response_meta, response] = handle_result(database, meta_proto, request, *user);
            respond(yield, response_meta, response);
            break;
        }
        default: {
            std::vector<char> buffer;
            buffer.resize(meta_proto.containersize());
            if (!direct_read(yield, buffer.data(), buffer.size()))
            {
                throw ResponseContainer::READ_ERROR;
            }

            auto [response_meta, response] = handle_new_job(database, meta_proto, buffer, *user);
            respond(yield, response_meta, response);
            break;
        }
    }
}

template <typename MESSAGE_TYPE>
MESSAGE_TYPE connection::read_message(boost::asio::yield_context &yield, size_t len)
{
    std::vector<char> recv_buffer(len);
    if (!direct_read(yield, recv_buffer.data(), recv_buffer.size()))
    {
        throw ResponseContainer::READ_ERROR;
    }

    if constexpr (!std::is_same_v<MESSAGE_TYPE, MetaData>)
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
        throw ResponseContainer::PROTO_PARSING_ERROR;
    }
    return msg;
}

template <class Serializable>
void connection::respond(boost::asio::yield_context &yield, const meta_data &meta_info,
                         const Serializable &message)
{
    namespace io = boost::iostreams;

    MetaData meta_proto;
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

    MetaData meta_proto;
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
                               ResponseContainer::StatusCode code)
{
    ResponseContainer response;
    response.set_status(code);
    respond(yield, meta_data{RequestType::ERROR}, response);
}

void connection::respond_error(boost::asio::yield_context &yield, ErrorType error_type)
{
    ErrorMessage msg;
    msg.set_type(error_type);
    respond(yield, meta_data{RequestType::ERROR}, msg);
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
