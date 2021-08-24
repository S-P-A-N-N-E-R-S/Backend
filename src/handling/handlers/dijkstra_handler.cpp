#include <chrono>
#include <handling/handlers/dijkstra_handler.hpp>
#include <networking/responses/response_factory.hpp>

namespace server {

DijkstraHandler::DijkstraHandler(std::unique_ptr<abstract_request> request)
    : m_request(std::move(request))
{
    shortest_path_request *sp_request = dynamic_cast<shortest_path_request *>(m_request.get());
    if (sp_request == nullptr)
    {
        throw std::runtime_error("dijkstra_handler: dynamic_cast failed!");
    }
    m_req_message = sp_request->graph_message();
    m_directed = false;
    m_start = m_req_message->node(sp_request->start_uid());
    m_graph = &m_req_message->graph();
    m_weights = sp_request->edge_costs();
    m_node_coords = sp_request->node_coords();
}

std::pair<graphs::ResponseContainer, long> DijkstraHandler::handle()
{
    ogdf::NodeArray<ogdf::edge> preds;
    ogdf::NodeArray<double> dist;

    auto start = std::chrono::high_resolution_clock::now();

    ogdf::Dijkstra<double>().call(*m_graph, *m_weights, m_start, preds, dist, m_directed);

    auto stop = std::chrono::high_resolution_clock::now();
    long ogdf_time = (std::chrono::duration_cast<std::chrono::microseconds>(stop - start)).count();

    auto out_graph = std::make_unique<ogdf::Graph>();
    auto out_node_uids = std::make_unique<ogdf::NodeArray<uid_t>>(*out_graph);
    auto out_edge_uids = std::make_unique<ogdf::EdgeArray<uid_t>>(*out_graph);

    auto out_edge_costs = std::make_unique<ogdf::EdgeArray<double>>(*out_graph);
    auto out_node_coords = std::make_unique<ogdf::NodeArray<server::node_coordinates>>(*out_graph);

    //to access nodes by uids
    auto original_uid_to_out_node = std::unordered_map<uid_t, ogdf::node>();

    // add all nodes to answer (before adding edges)
    for (auto n : m_graph->nodes)
    {
        auto new_node = out_graph->newNode();

        uid_t original_uid = m_req_message->node_uids()[n];

        (*out_node_uids)[new_node] = original_uid;
        (*out_node_coords)[new_node] = m_node_coords->operator[](n);

        original_uid_to_out_node[original_uid] = new_node;
    }

    //add edges to answer
    for (auto n : m_graph->nodes)
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

    std::unique_ptr<abstract_response> response = std::make_unique<shortest_path_response>(
        std::move(message), std::move(out_edge_costs), std::move(out_node_coords), status_code::OK);

    return std::pair<graphs::ResponseContainer, long>(
        response_factory().build_response(std::move(response)), ogdf_time);
}

graphs::HandlerInformation DijkstraHandler::handler_information()
{
    // add simple information
    auto information =
        DijkstraHandler::createHandlerInformation("Dijsktra", graphs::RequestType::SHORTEST_PATH);
    // add field information
    // DijkstraHandler::addFieldInformation(information, graphs::FieldInformation_FieldType_GRAPH,
    //                                      "Graph", "graph", true);
    // DijkstraHandler::addFieldInformation(information, graphs::FieldInformation_FieldType_VERTEX_ID,
    //                                      "Start node", "startIndex", true);
    // DijkstraHandler::addFieldInformation(information, graphs::FieldInformation_FieldType_VERTEX_ID,
    //                                      "End node", "endIndex");
    // DijkstraHandler::addFieldInformation(information, graphs::FieldInformation_FieldType_EDGE_COSTS,
    //                                      "Edge costs", "edgeCosts", true);
    // DijkstraHandler::addFieldInformation(information, graphs::FieldInformation_FieldType_EDGE_COSTS,
    //                                      "", "vertexCoordinates", true);
    // add result information
    // DijkstraHandler::addResultInformation(
    //     information, graphs::ResultInformation_HandlerReturnType_GRAPH, "graph");
    // DijkstraHandler::addResultInformation(
    //     information, graphs::ResultInformation_HandlerReturnType_EDGE_COSTS, "edgeCosts");
    // DijkstraHandler::addResultInformation(
    //     information, graphs::ResultInformation_HandlerReturnType_VERTEX_COORDINATES,
    //     "vertexCoordinates");
    return information;
}

}  // namespace server
