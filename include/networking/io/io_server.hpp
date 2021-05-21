#ifndef IO_SERVER_IO_SERVER_HPP
#define IO_SERVER_IO_SERVER_HPP

#include <boost/asio.hpp>
#include <future>

#include <networking/io/connection_handler.hpp>

namespace server {

/**
 * @brief Server class to provide async network io.
 *
 * Connections are handled via class <connection> and managed via <connection_handler>
 */
class io_server
{
    enum server_status { STOPPED, RUNNING };

public:
    explicit io_server(unsigned short listening_port);

    io_server(const io_server &) = delete;
    io_server &operator=(const io_server &) = delete;

    // If we want to move io_server objects, the io_context can not be a member
    io_server(io_server &&) = delete;
    io_server &operator=(io_server &&) = delete;

    ~io_server();

    /**
     * @brief Runs the io server until program aborts.
     *
     * Blocking function that runs the io server indefinitely.
     */
    void run();

    /**
     * @brief Starts the servers accept loop in the background.
     * @returns bool indicating if method was successful. False indicates that server is already running.
     *
     * Unlike the run method, start returns immediately and leaves the servers accept loop in the background.
     */
    bool start();

    /**
     * @brief Stops background accept loop.
     * @returns bool indicating if method was successful. False indicates that server is already stopped.
     */
    bool stop();

private:

    void accept();

    server_status m_status = STOPPED;

    boost::asio::io_context m_ctx;

    boost::asio::ip::tcp::acceptor m_acceptor;

    connection_handler m_connections;

    std::future<void> m_future;
};

}  // namespace server

#endif
