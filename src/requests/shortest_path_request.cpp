#include "networking/requests/shortest_path_request.hpp"

namespace server {

shortest_path_request::shortest_path_request(const graphs::ShortestPathRequest &proto_request)
    : abstract_request(request_type::SHORTEST_PATH)
    , m_graph_message{std::make_unique<server::graph_message>(proto_request.graph())}
    , m_start_index{proto_request.startindex()}
    , m_end_index{proto_request.endindex()}
    , m_edge_costs{std::make_unique<ogdf::EdgeArray<double>>(m_graph_message->graph())}
    , m_node_coords{std::make_unique<ogdf::NodeArray<node_coordinates>>(m_graph_message->graph())}
{
    for (const auto &[uid, cost] : proto_request.edgecosts())
    {
        const auto edge = m_graph_message->edge(uid);
        m_edge_costs->operator[](edge) = cost;
    }

    for (const auto &[uid, coords] : proto_request.vertexcoordinates())
    {
        const auto node = m_graph_message->node(uid);
        m_node_coords->operator[](node) = node_coordinates(coords);
    }
}

const graph_message *shortest_path_request::graph_message() const
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

const ogdf::EdgeArray<double> *shortest_path_request::edge_costs() const
{
    return this->m_edge_costs.get();
}

std::unique_ptr<ogdf::EdgeArray<double>> shortest_path_request::take_edge_costs()
{
    return std::move(this->m_edge_costs);
}

const ogdf::NodeArray<node_coordinates> *shortest_path_request::node_coords() const
{
    return this->m_node_coords.get();
}

std::unique_ptr<ogdf::NodeArray<node_coordinates>> shortest_path_request::take_node_coords()
{
    return std::move(this->m_node_coords);
}

}  // namespace server
