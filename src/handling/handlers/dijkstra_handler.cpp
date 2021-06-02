#include <handling/handlers/dijkstra_handler.hpp>

namespace server {

DijkstraHandler::DijkstraHandler(shortest_path_request &sp_request)
    : m_req_message(sp_request.graph_message())
    , m_directed(false)  //ToDo: add directed somewhere to graph or sp-request or both
    , m_start(m_req_message->node(sp_request.start_index()))
    , m_graph(m_req_message->graph())  //Maybe change graph() to pointer?
    , m_weights(ogdf::EdgeArray<int>(m_graph))
{
    for (auto e : m_graph.edges)
    {
        m_weights[e] = std::stoi(m_req_message->edge_attrs().at("cost")[e]);
    }
}

std::unique_ptr<abstract_response> DijkstraHandler::handle()
{
    ogdf::NodeArray<ogdf::edge> preds;
    ogdf::NodeArray<int> dist;

    ogdf::Dijkstra<int>().call(m_graph, m_weights, m_start, preds, dist, m_directed);

    auto out_graph = std::make_unique<ogdf::Graph>();
    auto out_node_uids = std::make_unique<ogdf::NodeArray<uid_t>>(*out_graph);
    auto out_edge_uids = std::make_unique<ogdf::EdgeArray<uid_t>>();
    auto out_coords = std::make_unique<ogdf::NodeArray<coords_t>>(*out_graph);
    auto out_graphMap = std::make_unique<GraphAttributeMap>();
    auto out_nodeMap = std::make_unique<NodeAttributeMap>();
    auto out_edgeMap = std::make_unique<EdgeAttributeMap>();

    auto out_preds = out_nodeMap->emplace("preds", ogdf::NodeArray<std::string>(*out_graph)).first;
    auto out_dist = out_nodeMap->emplace("dist", ogdf::NodeArray<std::string>(*out_graph)).first;
    for (auto n : m_graph.nodes)
    {
        auto new_node = out_graph->newNode();

        (*out_node_uids)[new_node] = m_req_message->node_uids()[n];

        if (preds[n] != nullptr)
        {
            out_preds->second[new_node] =
                std::to_string(m_req_message->node_uids()[preds[n]->opposite(n)]);
        }

        out_dist->second[new_node] = std::to_string(dist[n]);

        //This is redundant information we need so that response_factory does not crash!
        (*out_coords)[new_node] = m_req_message->node_coords()[n];
    }

    std::unique_ptr<graph_message> message(
        new graph_message(std::move(out_graph), std::move(out_node_uids), std::move(out_edge_uids),
                          std::move(out_coords), std::move(out_graphMap), std::move(out_nodeMap),
                          std::move(out_edgeMap)));
    
    return std::make_unique<shortest_path_response>(std::move(message), status_code::OK);
}

}  // namespace server
