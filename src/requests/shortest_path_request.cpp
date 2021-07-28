#include "networking/requests/shortest_path_request.hpp"

#include "networking/exceptions.hpp"

namespace server {

shortest_path_request::shortest_path_request(const graphs::ShortestPathRequest &proto_request)
    : abstract_request(request_type::SHORTEST_PATH)
    , m_graph_message{std::make_unique<server::graph_message>(proto_request.graph())}
    , m_start_uid{proto_request.startuid()}
    , m_end_uid{proto_request.enduid()}
    , m_edge_costs{std::make_unique<ogdf::EdgeArray<double>>(m_graph_message->graph())}
    , m_node_coords{std::make_unique<ogdf::NodeArray<node_coordinates>>(m_graph_message->graph())}
{
    if (proto_request.edgecosts_size() != this->m_graph_message->graph().numberOfEdges())
    {
        throw request_parse_error("Number of edge cost attributes does not match number of edges",
                                  this->m_type);
    }

    if (proto_request.vertexcoordinates_size() != this->m_graph_message->graph().numberOfNodes())
    {
        throw request_parse_error(
            "Number of vertex coordinate attributes does not match number of nodes", this->m_type);
    }

    // Convert attributes to OGDF-friendly representation
    for (size_t idx = 0; idx < proto_request.edgecosts_size(); ++idx)
    {
        const auto &edge = this->m_graph_message->all_edges()[idx];
        this->m_edge_costs->operator[](edge) = proto_request.edgecosts(idx);
    }

    for (size_t idx = 0; idx < proto_request.vertexcoordinates_size(); ++idx)
    {
        const auto &node = this->m_graph_message->all_nodes()[idx];
        this->m_node_coords->operator[](node) = proto_request.vertexcoordinates(idx);
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

uid_t shortest_path_request::start_uid() const
{
    return this->m_start_uid;
}

uid_t shortest_path_request::end_uid() const
{
    return this->m_end_uid;
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
