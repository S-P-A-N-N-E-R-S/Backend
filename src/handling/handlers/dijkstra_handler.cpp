#include "handling/handlers/dijkstra_handler.hpp"

#include <chrono>
#include "networking/responses/generic_response.hpp"
#include "networking/responses/response_factory.hpp"

namespace server {

dijkstra_handler::dijkstra_handler(std::unique_ptr<abstract_request> request)
    : m_request{}
{
    if (const auto *type_check_ptr = dynamic_cast<generic_request *>(request.get());
        !type_check_ptr)
    {
        throw std::runtime_error("dijkstra_handler: dynamic_cast failed!");
    }

    m_request = std::unique_ptr<generic_request>{static_cast<generic_request *>(request.release())};
}

std::pair<graphs::ResponseContainer, long> dijkstra_handler::handle()
{
    const auto *graph_message = m_request->graph_message();
    auto &graph = graph_message->graph();
    int start_uid = std::stoi(m_request->graph_attributes().at("startUid"));

    const auto *parsed_costs = m_request->edge_costs();

    const auto &origin = graph_message->node(start_uid);
    ogdf::NodeArray<ogdf::edge> preds;
    ogdf::NodeArray<double> dist;

    auto start = std::chrono::high_resolution_clock::now();

    ogdf::Dijkstra<double>().call(graph, *parsed_costs, origin, preds, dist);

    auto stop = std::chrono::high_resolution_clock::now();
    long ogdf_time = (std::chrono::duration_cast<std::chrono::microseconds>(stop - start)).count();

    // Build shortest path graph from origin to index
    auto spg = std::make_unique<ogdf::Graph>();
    auto sp_node_uids = std::make_unique<ogdf::NodeArray<uid_t>>(*spg);
    auto sp_edge_uids = std::make_unique<ogdf::EdgeArray<server::uid_t>>(*spg);
    auto sp_edge_costs = ogdf::EdgeArray<double>(*spg);
    auto sp_node_coords = ogdf::NodeArray<node_coordinates>(*spg);

    const auto &og_edge_uids = graph_message->edge_uids();
    const auto &og_node_uids = graph_message->node_uids();
    const auto *og_node_coords = m_request->node_coords();

    ogdf::NodeArray<ogdf::node> original_node_to_sp{graph};

    // add all nodes to answer (before adding edges)
    for (auto n : graph_message->graph().nodes)
    {
        const auto sp_node = spg->newNode();
        original_node_to_sp[n] = sp_node;
        (*sp_node_uids)[sp_node] = og_node_uids[n];
        sp_node_coords[sp_node] = (*og_node_coords)[n];
    }

    // add edges to answer
    for (auto n : graph_message->graph().nodes)
    {
        auto preds_edge = preds[n];
        if (preds_edge != nullptr)
        {
            auto source = original_node_to_sp[n];
            auto target = original_node_to_sp[preds_edge->opposite(n)];
            auto newEdge = spg->newEdge(source, target);
            (*sp_edge_uids)[newEdge] = graph_message->edge_uids()[preds_edge];
            sp_edge_costs[newEdge] = (*m_request->edge_costs())[preds_edge];
        }
    }

    server::graph_message spgm{std::move(spg), std::move(sp_node_uids), std::move(sp_edge_uids)};

    response_factory rspf;
    return std::make_pair(
        rspf.build_response(std::unique_ptr<abstract_response>{
            new generic_response{"dijkstra", &spgm, &sp_node_coords, &sp_edge_costs, nullptr,
                                 nullptr, nullptr, nullptr, nullptr, nullptr, status_code::OK}}),
        ogdf_time);
}

graphs::HandlerInformation dijkstra_handler::handler_information()
{
    // add simple information
    auto information =
        dijkstra_handler::createHandlerInformation("dijkstra", graphs::RequestType::GENERIC);
    // add field information
    dijkstra_handler::addFieldInformation(information, graphs::FieldInformation_FieldType_GRAPH,
                                          "Graph", "graph", true);
    dijkstra_handler::addFieldInformation(information, graphs::FieldInformation_FieldType_VERTEX_ID,
                                          "Start node", "graphAttributes.startUid", true);
    dijkstra_handler::addFieldInformation(information,
                                          graphs::FieldInformation_FieldType_EDGE_COSTS,
                                          "Edge costs", "edgeCosts", true);
    dijkstra_handler::addFieldInformation(information,
                                          graphs::FieldInformation_FieldType_VERTEX_COORDINATES, "",
                                          "vertexCoordinates", true);

    // add result information
    dijkstra_handler::addResultInformation(
        information, graphs::ResultInformation_HandlerReturnType_GRAPH, "graph");
    dijkstra_handler::addResultInformation(
        information, graphs::ResultInformation_HandlerReturnType_EDGE_COSTS, "edgeCosts");
    dijkstra_handler::addResultInformation(
        information, graphs::ResultInformation_HandlerReturnType_VERTEX_COORDINATES,
        "vertexCoordinates");
    return information;
}

}  // namespace server
