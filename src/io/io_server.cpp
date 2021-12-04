#include <networking/io/io_server.hpp>

#include <boost/asio/spawn.hpp>
#include <chrono>

#include <networking/io/connection.hpp>

namespace server {

using boost::asio::ip::tcp;
using boost::system::error_code;

io_server::io_server(unsigned short listening_port)
    : m_ctx{1}
    , m_ssl_ctx{boost::asio::ssl::context::tls}
    , m_acceptor{m_ctx, tcp::endpoint{tcp::v4(), listening_port}}
{
}

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
        while (m_status == RUNNING)
        {
            using ssl_socket = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
            ssl_socket sock{m_ctx, m_ssl_ctx};
            error_code err;
            m_acceptor.async_accept(sock.next_layer(), yield[err]);
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
                    // No error handling needed. Just catch the failed handshake.
                }
            }
        }
    });
}

}  // namespace server
