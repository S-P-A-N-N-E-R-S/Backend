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
    /// Enum representing the servers listening status
    enum server_status { STOPPED, RUNNING };

    io_server() = default;
    io_server(const io_server &) = delete;
    io_server &operator=(const io_server &) = delete;

    virtual ~io_server();

    /**
     * @brief Runs the io server until program aborts.
     *  Blocking function that runs the io server indefinitely.
     */
    virtual void run();

    /**
     * @brief Starts the servers accept loop in the background.
     *  Unlike the run method, start returns immediately and leaves the servers accept loop in the background.
     *
     * @returns bool indicating if method was successful. False indicates that server is already running.
     */
    virtual bool start();

    /**
     * @brief Stops background accept loop.
     *
     * @returns bool indicating if method was successful. False indicates that server is already stopped.
     */
    virtual bool stop();

    /**
     * @brief Pure virtual function to handle incoming i/o requests
     */
    virtual void handle() = 0;

    /// Listening status of the server
    server_status m_status = STOPPED;

    /// Future to keep the handling function running in the background when using start() function
    std::future<void> m_future;

    /// The servers i/o context is used for all async network operations
    boost::asio::io_context m_ctx{1};
};

}  // namespace server

#endif
