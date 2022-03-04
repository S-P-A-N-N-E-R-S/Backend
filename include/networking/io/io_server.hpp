#ifndef IO_SERVER_HPP
#define IO_SERVER_HPP

#include <boost/asio.hpp>
#include <future>

namespace server {

/**
 * @brief Interface for server classes performing I/O operations
 */
class io_server
{
protected:
    enum server_status { STOPPED, RUNNING };

    io_server() = default;
    io_server(const io_server &) = delete;
    io_server &operator=(const io_server &) = delete;

    virtual ~io_server();

    /**
     * @brief Runs the io server until program aborts.
     *
     * Blocking function that runs the io server indefinitely.
     */
    virtual void run();

    /**
     * @brief Starts the servers accept loop in the background.
     * @returns bool indicating if method was successful. False indicates that server is already running.
     *
     * Unlike the run method, start returns immediately and leaves the servers accept loop in the background.
     */
    virtual bool start();

    /**
     * @brief Stops background accept loop.
     * @returns bool indicating if method was successful. False indicates that server is already stopped.
     */
    virtual bool stop();

    /**
     *
     */
    virtual void handle() = 0;

    server_status m_status = STOPPED;

    std::future<void> m_future;

    boost::asio::io_context m_ctx{1};
};

}  // namespace server

#endif
