#ifndef IO_MANAGEMENT_SERVER_HPP
#define IO_MANAGEMENT_SERVER_HPP

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <future>
#include <nlohmann/json.hpp>
#include <string_view>
#include <utility>

#include <networking/io/io_server.hpp>

namespace server {

/**
 * @brief Server class to provide async network io for the management API.
 */
class management_server : public io_server
{
public:
    /**
     * @brief Constructs a new management_server instance from a UNIX domain socket
     *
     * @param descriptor String_view containing the name of a UNIX file descriptor used for
     *  communication
     */
    explicit management_server(std::string_view descriptor);

    management_server(const management_server &) = delete;
    management_server &operator=(const management_server &) = delete;
    management_server(management_server &&) = delete;
    management_server &operator=(management_server &&) = delete;

    ~management_server();

    // Use functions provided by abstract base class
    using io_server::run;
    using io_server::start;
    using io_server::stop;

private:
    /// Handle incoming connections by creating a client_connection per request
    void handle() override;

    /// Handle incoming JSON request from spannersctl CLI tool
    void handle_request(boost::asio::yield_context &yield,
                        const boost::asio::local::datagram_protocol::endpoint &receiver,
                        nlohmann::json request);

    /// Read and parse JSON data from underlying socket connection
    std::pair<boost::asio::local::datagram_protocol::endpoint, nlohmann::json> read_json(
        boost::asio::yield_context &yield);

    /// Respond with JSON data to spannersctl CLI tool
    void respond_json(boost::asio::yield_context &yield,
                      const boost::asio::local::datagram_protocol::endpoint &receiver,
                      const nlohmann::json &response);

    /// Underlying local socket used for communications
    boost::asio::local::datagram_protocol::socket m_sock;
};

}  // namespace server

#endif
