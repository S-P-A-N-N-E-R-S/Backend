#include <networking/io/io_server.hpp>

#include <chrono>

#include <networking/io/connection.hpp>

namespace server {

using boost::asio::ip::tcp;
using boost::system::error_code;

io_server::io_server(unsigned short listening_port)
    : m_ctx{1}
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
        accept();
        m_ctx.run();
    }
}

bool io_server::start()
{
    if (m_status != RUNNING)
    {
        m_future = std::async(std::launch::async, [this]() {
            accept();
            m_ctx.run();
        });

        m_status = RUNNING;
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
    m_acceptor.async_accept([this](error_code err, tcp::socket sock) {
        if (!err)
        {
            // Use timestamp to identify the connection
            using namespace std::chrono;
            size_t id = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
            m_connections.add(id, std::make_unique<connection>(id, m_connections, std::move(sock)));
        }

        accept();
    });
}

}  // namespace server
