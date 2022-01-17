#include <networking/io/client_server.hpp>

#include <boost/asio/spawn.hpp>
#include <chrono>
#include <iostream>

#include <networking/io/client_connection.hpp>

namespace server {

using boost::asio::ip::tcp;
using boost::asio::ssl::stream;
using boost::system::error_code;

#ifdef SPANNERS_UNENCRYPTED_CONNECTION
client_server::client_server(unsigned short listening_port)
    : io_server{}
    , m_acceptor{m_ctx, tcp::endpoint{tcp::v6(), listening_port}}
    , m_connections{}
{
}
#else
client_server::client_server(unsigned short listening_port, const std::string &cert_path,
                             const std::string &key_path)
    : io_server{}
    , m_ssl_ctx{boost::asio::ssl::context::tls}
    , m_acceptor{m_ctx, tcp::endpoint{tcp::v6(), listening_port}}
    , m_connections{}
{
    // Configure ssl context
    m_ssl_ctx.set_options(boost::asio::ssl::context::default_workarounds);
    m_ssl_ctx.use_certificate_chain_file(cert_path);
    m_ssl_ctx.use_private_key_file(key_path, boost::asio::ssl::context::pem);
}
#endif

void client_server::handle()
{
    boost::asio::spawn(m_ctx, [this](boost::asio::yield_context yield) {
#ifdef SPANNERS_UNENCRYPTED_CONNECTION
        std::cout << "[INFO] Listening for unencrypted connections from clients on "
                  << m_acceptor.local_endpoint().port() << '\n';
#else
        std::cout << "[INFO] Listening for encrypted connections from clients on "
                  << m_acceptor.local_endpoint().port() << '\n';
#endif

        while (m_status == RUNNING)
        {
#ifndef SPANNERS_UNENCRYPTED_CONNECTION
            client_connection::socket_ptr sock =
                std::make_unique<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>(m_ctx,
                                                                                         m_ssl_ctx);
            error_code err;
            m_acceptor.async_accept(sock->next_layer(), yield[err]);
#else
            client_connection::socket_ptr sock =
                std::make_unique<boost::asio::ip::tcp::socket>(m_ctx);
            error_code err;
            m_acceptor.async_accept(*sock, yield[err]);
#endif

            if (!err)
            {
                using namespace std::chrono;
                size_t id =
                    duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                try
                {
                    m_connections.add(id, std::make_unique<client_connection>(id, m_connections,
                                                                              std::move(sock)));
                }
                catch (std::runtime_error &err)
                {
                    std::cout << "[ERROR] Connection to client failed: " << err.what() << '\n';
                    // No error handling needed. Just catch the failed connection establishment
                }
            }
        }
    });
}

}  // namespace server
