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
    explicit client_server(unsigned short listening_port);
#else
    client_server(unsigned short listening_port, const std::string &cert_path,
                  const std::string &key_path);
#endif

    client_server(const client_server &) = delete;
    client_server &operator=(const client_server &) = delete;
    client_server(client_server &&) = delete;
    client_server &operator=(client_server &&) = delete;

    ~client_server() = default;

    using io_server::run;
    using io_server::start;
    using io_server::stop;

private:
    void handle() override;

#ifndef SPANNERS_UNENCRYPTED_CONNECTION
    boost::asio::ssl::context m_ssl_ctx;
#endif

    boost::asio::ip::tcp::acceptor m_acceptor;

    connection_handler<client_connection> m_connections;
};

}  // namespace server

#endif
