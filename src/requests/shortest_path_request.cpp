#include "networking/requests/shortest_path_request.hpp"

namespace server {

shortest_path_request::shortest_path_request(const graphs::ShortPathRequest &proto_request)
    : abstract_request(request_type::SHORTEST_PATH)
    , m_graph_message(std::make_unique<server::graph_message>(proto_request.graph()))
    , m_start_index{proto_request.startindex()}
    , m_end_index{proto_request.endindex()}
{
}

const graph_message *shortest_path_request::graph_message()
{
    return this->m_graph_message.get();
}

std::unique_ptr<graph_message> shortest_path_request::take_graph_message()
{
    return std::move(this->m_graph_message);
}

uid_t shortest_path_request::start_index() const
{
    return this->m_start_index;
}

uid_t shortest_path_request::end_index() const
{
    return this->m_end_index;
}

}  // namespace server
