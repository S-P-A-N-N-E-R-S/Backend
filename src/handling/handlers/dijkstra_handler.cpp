#include <handling/handlers/dijkstra_handler.hpp>

namespace server {

DijkstraHandler::DijkstraHandler(shortest_path_request &sp_request)
    : m_req_message(sp_request.graph_message())
    , m_directed(false)  //ToDo: add directed somewhere to graph or sp-request or both
    , m_start(m_req_message->node(sp_request.start_index()))
    , m_graph(m_req_message->graph())  //Maybe change graph() to pointer?
    , m_weights(sp_request.edge_costs())
    , m_node_coords(sp_request.node_coords())
{
}

std::unique_ptr<abstract_response> DijkstraHandler::handle()
{
    ogdf::NodeArray<ogdf::edge> preds;
    ogdf::NodeArray<double> dist;

    ogdf::Dijkstra<double>().call(m_graph, *m_weights, m_start, preds, dist, m_directed);

    auto out_graph = std::make_unique<ogdf::Graph>();
    auto out_node_uids = std::make_unique<ogdf::NodeArray<uid_t>>(*out_graph);
    auto out_edge_uids = std::make_unique<ogdf::EdgeArray<uid_t>>(*out_graph);

    auto out_edge_costs = std::make_unique<ogdf::EdgeArray<double>>(*out_graph);
    auto out_node_coords = std::make_unique<ogdf::NodeArray<server::node_coordinates>>(*out_graph);

    //to access nodes by uids
    auto original_uid_to_out_node = std::unordered_map<uid_t, ogdf::node>();

    // add all nodes to answer (before adding edges)
    for (auto n : m_graph.nodes)
    {
        auto new_node = out_graph->newNode();

        uid_t original_uid = m_req_message->node_uids()[n];

        (*out_node_uids)[new_node] = original_uid;
        (*out_node_coords)[new_node] = m_node_coords->operator[](n);

        original_uid_to_out_node[original_uid] = new_node;
    }

    //add edges to answer
    for (auto n : m_graph.nodes)
    {
        auto preds_edge = preds[n];
        if (preds_edge != nullptr)
        {
            auto source_uid = m_req_message->node_uids()[n];
            auto target_uid = m_req_message->node_uids()[preds_edge->opposite(n)];
            auto newEdge = out_graph->newEdge(original_uid_to_out_node[source_uid],
                                              original_uid_to_out_node[target_uid]);
            (*out_edge_uids)[newEdge] = m_req_message->edge_uids()[preds_edge];
            (*out_edge_costs)[newEdge] = m_weights->operator[](preds_edge);
        }
    }

    auto message = std::make_unique<graph_message>(std::move(out_graph), std::move(out_node_uids),
                                                   std::move(out_edge_uids));

    return std::make_unique<shortest_path_response>(std::move(message), std::move(out_edge_costs),
                                                    std::move(out_node_coords), status_code::OK);
}

}  // namespace server