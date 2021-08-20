#include "networking/responses/shortest_path_response.hpp"

namespace server {

shortest_path_response::shortest_path_response(
    std::unique_ptr<server::graph_message> shortest_path,
    std::unique_ptr<ogdf::EdgeArray<double>> edge_costs,
    std::unique_ptr<ogdf::NodeArray<node_coordinates>> node_coords, status_code status)
    : abstract_response(response_type::SHORTEST_PATH, status)
    , m_proto_graph(std::make_unique<graphs::Graph>())
    , m_edge_costs(std::make_unique<google::protobuf::RepeatedField<double>>())
    , m_vertex_coords(
          std::make_unique<google::protobuf::RepeatedPtrField<graphs::VertexCoordinates>>())
{
    if (status != status_code::OK)
    {
        return;
    }

    // If status code is OK, we check some correctness conditions.

    if (shortest_path)
    {
        this->m_proto_graph = std::make_unique<graphs::Graph>(*(shortest_path->as_proto()));
    }

    if (edge_costs)
    {
        const auto nof_edges = shortest_path->graph().numberOfEdges();
        const auto &all_edges = shortest_path->all_edges();

        this->m_edge_costs->Reserve(nof_edges);
        for (size_t idx = 0; idx < nof_edges; ++idx)
        {
            const auto cost = edge_costs->operator[](all_edges[idx]);
            this->m_edge_costs->Add(cost);
        }
    }

    if (node_coords)
    {
        const auto nof_nodes = shortest_path->graph().numberOfNodes();
        const auto &all_nodes = shortest_path->all_nodes();

        this->m_vertex_coords->Reserve(nof_nodes);
        for (size_t idx = 0; idx < nof_nodes; ++idx)
        {
            auto coords = node_coords->operator[](all_nodes[idx]).as_proto();
            this->m_vertex_coords->Add(std::move(coords));
        }
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

const google::protobuf::RepeatedField<double> *shortest_path_response::edge_costs() const
{
    return this->m_edge_costs.get();
}

std::unique_ptr<google::protobuf::RepeatedField<double>> shortest_path_response::take_edge_costs()
{
    return std::move(this->m_edge_costs);
}

const google::protobuf::RepeatedPtrField<graphs::VertexCoordinates>
    *shortest_path_response::vertex_coords() const
{
    return this->m_vertex_coords.get();
}

std::unique_ptr<google::protobuf::RepeatedPtrField<graphs::VertexCoordinates>>
    shortest_path_response::take_vertex_coords()
{
    return std::move(this->m_vertex_coords);
}

}  // namespace server
