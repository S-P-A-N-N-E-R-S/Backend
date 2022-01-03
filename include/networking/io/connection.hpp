#ifndef IO_SERVER_CONNECTION_HPP
#define IO_SERVER_CONNECTION_HPP

#include <array>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ssl.hpp>
#include <memory>
#include <vector>

#include <networking/messages/meta_data.hpp>
#include <persistence/database_wrapper.hpp>  // for binary_data

#include "container.pb.h"
#include "error.pb.h"
#include "meta.pb.h"

namespace server {

class connection_handler;

/**
 * @brief Representaion of a tcp connection communicating with gzip compressed protobuf messages.
 *
 * A connections lifetime is always managed by a <server::connection_handler>
 *
 * A successful connection receives a <graphs::RequestContainer> and always responds with a
 *  <graphs::ResponseContainer> protobuf. Request and response are gzip compressed and serialized
 *  protobuf messages.
 */
class connection
{
public:
#ifndef SPANNERS_UNENCRYPTED_CONNECTION
    using socket_ptr = std::unique_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>;
#else
    using socket_ptr = std::unique_ptr<boost::asio::ip::tcp::socket>;
#endif

    /**
     * @brief Ctor of a connection instance
     * @param id Identifier of the connection in the corresponding <server::connection_handler>.
     *          Used to destruct the connection when handling is finished
     * @param connection_handler Reference to the lifetime managing <server::connection_handler>.
     * @param socket Underlying socket of the connection.
     */
    explicit connection(size_t id, connection_handler &handler, socket_ptr sock);

    connection(const connection &) = delete;
    connection &operator=(const connection &) = delete;
    connection(connection &&rhs) = delete;
    connection &operator=(connection &&rhs) = delete;

    ~connection() = default;

    /**
     * @brief Handles receiving and responding.
     *
     * Creates a boost coroutine to handle the request and respond to the client
     */
    void handle();

private:
    void handle_internal(boost::asio::yield_context &yield);

    void handle_available_handlers(boost::asio::yield_context &yield);

    void handle_status(boost::asio::yield_context &yield, database_wrapper &db, const user &user);

    void handle_result(boost::asio::yield_context &yield, database_wrapper &db,
                       const graphs::MetaData &meta, const user &user);

    void handle_new_job(boost::asio::yield_context &yield, database_wrapper &db,
                        const graphs::MetaData &meta, const user &user);

    template <typename MESSAGE_TYPE>
    MESSAGE_TYPE read_message(boost::asio::yield_context &yield, size_t len);

    template <class Serializable>
    void respond(boost::asio::yield_context &yield, const meta_data &meta_info,
                 const Serializable &container);

    /// Overload when responding with a message retrieved directly from the database
    void respond(boost::asio::yield_context &yield, const meta_data &meta_info,
                 const binary_data &binary);

    void respond_error(boost::asio::yield_context &yield,
                       graphs::ResponseContainer_StatusCode code);
    void respond_error(boost::asio::yield_context &yield, graphs::ErrorType error_type);

    bool direct_read(boost::asio::yield_context &yield, char *const data, size_t length);

    void direct_write(boost::asio::yield_context &yield, const char *data, size_t length);

    size_t m_identifier;

    connection_handler &m_handler;

    socket_ptr m_sock;
};

}  // namespace server

#endif
