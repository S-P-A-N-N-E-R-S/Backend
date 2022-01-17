#pragma once

#include <string_view>
#include <variant>

#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

using boost::asio::local::datagram_protocol;
using namespace std::literals::string_view_literals;

namespace cli {

class io
{
public:

    static io &instance();

    void send(const nlohmann::json &msg);
    nlohmann::json receive();

private:
    static constexpr auto LOCAL_EP_PATH = "/tmp/spanners_client"sv;
    static constexpr auto REMOTE_EP_PATH = "/tmp/spanners_server"sv;

    io();
    ~io();

    io(const io &other) = delete;
    io(io &&other) = delete;
    io operator=(const io &other) = delete;
    io operator=(io &&other) = delete;

    datagram_protocol::endpoint m_local_ep;
    datagram_protocol::endpoint m_remote_ep;
    boost::asio::io_context m_ctx;
    datagram_protocol::socket m_sock;
};

}  // namespace cli
