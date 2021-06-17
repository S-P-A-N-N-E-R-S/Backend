#include "networking/responses/shortest_path_response.hpp"

namespace server {

shortest_path_response::shortest_path_response(
    std::unique_ptr<server::graph_message> shortest_path,
    std::unique_ptr<ogdf::EdgeArray<double>> edge_costs,
    std::unique_ptr<ogdf::NodeArray<node_coordinates>> node_coords, status_code status)
    : abstract_response(response_type::SHORTEST_PATH, status)
    , m_proto_graph(std::make_unique<graphs::Graph>())
    , m_edge_costs(std::make_unique<google::protobuf::Map<uid_t, double>>())
    , m_vertex_coords(std::make_unique<google::protobuf::Map<uid_t, graphs::VertexCoordinates>>())
{
    this->m_proto_graph->set_uid(shortest_path->uid());

    const ogdf::Graph &graph = shortest_path->graph();

    const auto &node_uids = shortest_path->node_uids();
    for (const ogdf::node &node : graph.nodes)
    {
        const uid_t uid = node_uids[node];

        auto *inserted = this->m_proto_graph->add_vertexlist();
        inserted->set_uid(uid);

        this->m_vertex_coords->operator[](uid) = node_coords->operator[](node).as_proto();
    }

    const auto &edge_uids = shortest_path->edge_uids();
    for (const ogdf::edge &edge : graph.edges)
    {
        const uid_t uid = edge_uids[edge];

        auto *inserted = this->m_proto_graph->add_edgelist();
        inserted->set_uid(uid);
        inserted->set_invertexuid(node_uids[edge->source()]);
        inserted->set_outvertexuid(node_uids[edge->target()]);

        this->m_edge_costs->operator[](uid) = edge_costs->operator[](edge);
    }
}

const graphs::Graph *shortest_path_response::proto_graph() const
{
    return this->m_proto_graph.get();
}

std::unique_ptr<graphs::Graph> shortest_path_response::take_proto_graph()
{
    return std::move(this->m_proto_graph);
}

const google::protobuf::Map<uid_t, double> *shortest_path_response::edge_costs() const
{
    return this->m_edge_costs.get();
}

std::unique_ptr<google::protobuf::Map<uid_t, double>> shortest_path_response::take_edge_costs()
{
    return std::move(this->m_edge_costs);
}

const google::protobuf::Map<uid_t, graphs::VertexCoordinates>
    *shortest_path_response::vertex_coords() const
{
    return this->m_vertex_coords.get();
}

std::unique_ptr<google::protobuf::Map<uid_t, graphs::VertexCoordinates>>
    shortest_path_response::take_vertex_coords()
{
    return std::move(this->m_vertex_coords);
}

}  // namespace server
