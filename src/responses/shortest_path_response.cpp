#include "networking/responses/shortest_path_response.hpp"

namespace server {

shortest_path_response::shortest_path_response(std::unique_ptr<server::graph_message> shortest_path,
                                               status_code status)
    : abstract_response(response_type::SHORTEST_PATH, status)
    , m_proto_graph(std::make_unique<graphs::Graph>())
{
    this->m_proto_graph->set_uid(shortest_path->uid());

    const ogdf::Graph &graph = shortest_path->graph();

    const auto &node_uids = shortest_path->node_uids();
    for (const ogdf::node &node : graph.nodes)
    {
        const uid_t uid = node_uids[node];

        auto *inserted = this->m_proto_graph->add_vertexlist();
        inserted->set_uid(uid);

        const coords_t &coords = shortest_path->node_coords()[node];
        inserted->set_x(std::get<0>(coords));
        inserted->set_y(std::get<1>(coords));
        inserted->set_z(std::get<2>(coords));

        auto *proto_attrs = inserted->mutable_attributes();

        for (const auto &[attr_name, attrs_for_nodes] : shortest_path->node_attrs())
        {
            (*proto_attrs)[attr_name] = attrs_for_nodes[node];
        }
    }

    const auto &edge_uids = shortest_path->edge_uids();
    for (const ogdf::edge &edge : graph.edges)
    {
        const uid_t uid = edge_uids[edge];

        auto *inserted = this->m_proto_graph->add_edgelist();
        inserted->set_uid(uid);
        inserted->set_invertexuid(node_uids[edge->source()]);
        inserted->set_outvertexuid(node_uids[edge->target()]);

        auto *proto_attrs = inserted->mutable_attributes();

        for (const auto &[attr_name, attrs_for_edges] : shortest_path->edge_attrs())
        {
            (*proto_attrs)[attr_name] = attrs_for_edges[edge];
        }
    }

    auto *attrs = this->m_proto_graph->mutable_attributes();
    for (const auto &[key, val] : shortest_path->graph_attrs())
    {
        (*attrs)[key] = val;
    }
}

const graphs::Graph *shortest_path_response::proto_graph()
{
    return this->m_proto_graph.get();
}

std::unique_ptr<graphs::Graph> shortest_path_response::take_proto_graph()
{
    return std::move(this->m_proto_graph);
}

}  // namespace server
