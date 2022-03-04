#include <networking/io/io_server.hpp>

#include <iostream>

namespace server {

io_server::~io_server()
{
    m_ctx.stop();
    if (m_future.valid())
    {
        m_future.get();
    }
}

void io_server::run()
{
    if (m_status != RUNNING)
    {
        m_status = RUNNING;
        this->handle();
        m_ctx.run();
    }
}

bool io_server::start()
{
    if (m_status != RUNNING)
    {
        m_future = std::async(std::launch::async, [this]() {
            m_status = RUNNING;
            this->handle();
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

}  // namespace server
