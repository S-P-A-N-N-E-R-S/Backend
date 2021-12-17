#include <networking/io/io_server.hpp>

#include <boost/asio/spawn.hpp>
#include <chrono>
#include <iostream>
#include <string_view>

#include <networking/io/connection.hpp>

namespace server {

using boost::asio::ip::tcp;
using boost::asio::ssl::stream;
using boost::system::error_code;

#ifdef SPANNERS_UNENCRYPTED_CONNECTION
io_server::io_server(unsigned short listening_port)
    : m_ctx{1}
    , m_acceptor{m_ctx, tcp::endpoint{tcp::v6(), listening_port}}
{
}
#else
io_server::io_server(unsigned short listening_port, const std::string &cert_path,
                     const std::string &key_path)
    : m_ctx{1}
    , m_ssl_ctx{boost::asio::ssl::context::tls}
    , m_acceptor{m_ctx, tcp::endpoint{tcp::v6(), listening_port}}
{
    // Configure ssl context
    m_ssl_ctx.set_options(boost::asio::ssl::context::default_workarounds);
    // TODO Make this pahts configurable
    m_ssl_ctx.use_certificate_chain_file(cert_path);
    m_ssl_ctx.use_private_key_file(key_path, boost::asio::ssl::context::pem);
}
#endif

io_server::~io_server()
{
    m_ctx.stop();
}

void io_server::run()
{
    if (m_status != RUNNING)
    {
        m_status = RUNNING;
        accept();
        m_ctx.run();
    }
}

bool io_server::start()
{
    if (m_status != RUNNING)
    {
        m_future = std::async(std::launch::async, [this]() {
            m_status = RUNNING;
            accept();
            m_ctx.run();
        });

        return true;
    }
    return false;
}

bool io_server::stop()
{
    if (m_status == RUNNING)
    {
        m_ctx.stop();
        m_future.get();

        m_status = STOPPED;
        return true;
    }
    return false;
}

void io_server::accept()
{
    boost::asio::spawn(m_ctx, [this](boost::asio::yield_context yield) {
#ifdef SPANNERS_UNENCRYPTED_CONNECTION
        std::cout << "[SERVER] Listening for unencrypted connections\n";
#else
        std::cout << "[SERVER] Listening for encrypted connections\n";
#endif

        while (m_status == RUNNING)
        {
#ifdef SPANNERS_UNENCRYPTED_CONNECTION
            tcp::socket sock{m_ctx};
            error_code err;
            m_acceptor.async_accept(sock, yield[err]);
#else
            stream<tcp::socket> sock{m_ctx, m_ssl_ctx};
            error_code err;
            m_acceptor.async_accept(sock.next_layer(), yield[err]);
#endif

            if (!err)
            {
                using namespace std::chrono;
                size_t id =
                    duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                try
                {
                    m_connections.add(
                        id, std::make_unique<connection>(id, m_connections, std::move(sock)));
                }
                catch (std::runtime_error &err)
                {
                    std::cout << "[SERVER] Connection failed: " << err.what() << '\n';
                    // No error handling needed. Just catch the failed connection establishment
                }
            }
        }
    });
}

}  // namespace server
