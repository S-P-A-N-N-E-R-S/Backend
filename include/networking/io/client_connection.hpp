#ifndef IO_SERVER_CLIENT_CONNECTION_HPP
#define IO_SERVER_CLIENT_CONNECTION_HPP

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

template <typename CONNECTION_TYPE>
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
class client_connection
{
public:
#ifndef SPANNERS_UNENCRYPTED_CONNECTION
    using socket_ptr = std::unique_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>;
#else
    using socket_ptr = std::unique_ptr<boost::asio::ip::tcp::socket>;
#endif

    /**
     * @brief Ctor of a connection instance for the client api
     * @param id Identifier of the connection in the corresponding <server::connection_handler>.
     *          Used to destruct the connection when handling is finished
     * @param connection_handler Reference to the lifetime managing <server::connection_handler>.
     * @param socket Underlying socket of the connection.
     */
    explicit client_connection(size_t id, connection_handler<client_connection> &handler,
                               socket_ptr sock);

    client_connection(const client_connection &) = delete;
    client_connection &operator=(const client_connection &) = delete;
    client_connection(client_connection &&rhs) = delete;
    client_connection &operator=(client_connection &&rhs) = delete;

    ~client_connection() = default;

    /**
     * @brief Handles receiving and responding.
     *  Creates a boost coroutine to handle the request and respond to the client.
     *  When finished this function will automatically deregister the connection from m_handler
     */
    void handle();

private:
    /// Internal handling of the connection
    void handle_internal(boost::asio::yield_context &yield);

    /// Read a protobuf message from the underlying socket connection and parse it according to the template type
    template <typename MESSAGE_TYPE>
    MESSAGE_TYPE read_message(boost::asio::yield_context &yield, size_t len);

    /// Respond with a serializable in form of a protobuf message
    template <class Serializable>
    void respond(boost::asio::yield_context &yield, const meta_data &meta_info,
                 const Serializable &container);

    /// Respond with binary data (used for responses read from the database)
    void respond(boost::asio::yield_context &yield, const meta_data &meta_info,
                 const binary_data &binary);

    /// Respond an error response with a graphs::ResponseContainer containing the given status code as its body
    void respond_error(boost::asio::yield_context &yield,
                       graphs::ResponseContainer::StatusCode code);

    /// Respond an error response with a graphs::ErrorMessage as its body
    void respond_error(boost::asio::yield_context &yield, graphs::ErrorType error_type);

    /// Read data from the underlying socket connection
    bool direct_read(boost::asio::yield_context &yield, char *const data, size_t length);

    /// Write data to the underlying socket connection
    void direct_write(boost::asio::yield_context &yield, const char *data, size_t length);

    /// Identifier used to identify the connection within m_handler
    size_t m_identifier;

    /// connection_handler handles the lifetime of this connection
    connection_handler<client_connection> &m_handler;

    /// Underlying socket for network communications
    socket_ptr m_sock;
};

}  // namespace server

#endif
