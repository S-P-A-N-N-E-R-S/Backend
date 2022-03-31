#ifndef IO_CLIENT_SERVER_HPP
#define IO_CLIENT_SERVER_HPP

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <future>

#include <networking/io/client_connection.hpp>
#include <networking/io/connection_handler.hpp>
#include <networking/io/io_server.hpp>

namespace server {

/**
 * @brief Server class to provide async network io.
 *
 * Connections are handled via class <connection> and managed via <connection_handler>
 */
class client_server : public io_server
{
public:
#ifdef SPANNERS_UNENCRYPTED_CONNECTION
    /**
     * @brief Constructor for client server serving unencrypted connections
     *
     * @param listening_port Port on which the server should listen for incoming connections
     */
    explicit client_server(unsigned short listening_port);
#else
    /**
     * @brief Constructor for client server server encrypted connections
     *
     * @param listening_port Port on which the server short listen for incoming connections
     * @param cert_path Path to the servers PEM encoded public certificate used for TLS connections
     * @param key_path Path to the servers PEM encoded private key used for TLS connections
     */
    client_server(unsigned short listening_port, const std::string &cert_path,
                  const std::string &key_path);
#endif

    client_server(const client_server &) = delete;
    client_server &operator=(const client_server &) = delete;
    client_server(client_server &&) = delete;
    client_server &operator=(client_server &&) = delete;

    ~client_server() = default;

    // Use functions provided by abstract base class
    using io_server::run;
    using io_server::start;
    using io_server::stop;

private:
    /// Handle incoming connections by creating a client_connection per request
    void handle() override;

#ifndef SPANNERS_UNENCRYPTED_CONNECTION
    /// TLS context only used for encrypted connections
    boost::asio::ssl::context m_ssl_ctx;
#endif

    /// Underlying socket connection
    boost::asio::ip::tcp::acceptor m_acceptor;

    /// Storage to keep active connections alive
    connection_handler<client_connection> m_connections;
};

}  // namespace server

#endif
