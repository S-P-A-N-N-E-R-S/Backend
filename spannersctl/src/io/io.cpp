#include "io/io.hpp"

using boost::asio::io_context;
using boost::asio::local::datagram_protocol;
using nlohmann::json;

namespace cli {

io &io::instance()
{
    static io instance;

    return instance;
};

io::io()
    : m_local_ep{LOCAL_EP_PATH}
    , m_remote_ep{REMOTE_EP_PATH}
    , m_ctx{}
    , m_sock{[this] {                    // TODO(leon): Maybe we should do this in the dtor actually
        ::unlink(LOCAL_EP_PATH.data());  // This is only safe because we know it's null-terminated
        return datagram_protocol::socket{m_ctx, m_local_ep};
    }()}
{
}

io::~io()
{
    // Unlink the local socket on destruction
    datagram_protocol::socket::endpoint_type endpoint = m_sock.local_endpoint();
    m_sock.release();
    m_sock.close();
    ::unlink(endpoint.path().c_str());
}

void io::send(const json &msg)
{
    const auto serialized{msg.dump()};
    size_t sent_so_far = 0;

    do
    {
        const auto sent = m_sock.send_to(
            boost::asio::buffer(serialized.data() + sent_so_far, serialized.size() - sent_so_far),
            m_remote_ep);
        sent_so_far += sent;
    } while (sent_so_far < serialized.size());
}

json io::receive()
{
    m_sock.wait(datagram_protocol::socket::wait_read);

    std::string resp;

    auto avbl = m_sock.available();
    while (avbl > 0)
    {
        std::vector<char> buf;
        buf.reserve(avbl);
        m_sock.receive(boost::asio::buffer(buf.data(), avbl));

        resp += std::string_view{buf.data(), avbl};
        avbl = m_sock.available();
    }

    return json::parse(resp);

}
}  // namespace cli
