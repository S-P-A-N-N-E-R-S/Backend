#ifndef IO_SERVER_CONNECTION_HPP
#define IO_SERVER_CONNECTION_HPP

#include <array>
#include <boost/asio.hpp>
#include <vector>

#include "container.pb.h"

namespace server {

class connection_handler;

/**
 * @brief Representaion of a tcp connection communicating with gzip compressed protobuf messages.
 *
 * A connections lifetime is always managed by a <server::connection_handler>
 *
 * A successful connection receives a <graphs::RequestContainer> and always responds with a
 *  <graphs::ResponseContainer> protobuf. Request and response are gzip compressed and serialized
 *  protobuf messages.
 */
class connection
{
public:
    /**
     * @brief Ctor of a connection instance
     * @param id Identifier of the connection in the corresponding <server::connection_handler>.
     *          Used to destruct the connection when handling is finished
     * @param connection_handler Reference to the lifetime managing <server::connection_handler>.
     * @param socket Underlying socket of the connection.
     */
    explicit connection(size_t id, connection_handler &handler,
                        boost::asio::ip::tcp::socket socket);

    connection(const connection &) = delete;
    connection &operator=(const connection &) = delete;
    connection(connection &&rhs) = delete;
    connection &operator=(connection &&rhs) = delete;

    ~connection();

    /**
     * @brief Handles receiving and responding.
     *
     * Starts async handling process and returns immediately.
     */
    void handle();

private:
    void read();

    void respond(graphs::ResponseContainer &container);

    void write(const char *buffer, size_t length);

    size_t m_identifier;

    connection_handler &m_handler;

    boost::asio::ip::tcp::socket m_sock;

    uint64_t m_size{0};

    std::vector<char> m_buffer;
};

}  // namespace server

#endif
