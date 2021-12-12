#include <handling/handlers/kruskal_handler.hpp>

#include <ogdf/basic/GraphCopy.h>
#include <ogdf/basic/extended_graph_alg.h>

#include <chrono>
#include "networking/exceptions.hpp"
#include "networking/responses/generic_response.hpp"
#include "networking/responses/response_factory.hpp"

namespace server {

std::string kruskal_handler::name()
{
    return "Kruskals Algorithm";
}

graphs::HandlerInformation kruskal_handler::handler_information()
{
    auto information = createHandlerInformation(name(), graphs::RequestType::GENERIC);

    addFieldInformation(information, graphs::FieldInformation_FieldType_GRAPH, "Graph", "graph",
                        true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_EDGE_COSTS, "Edge costs",
                        "edgeCosts", true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_VERTEX_COORDINATES, "",
                        "vertexCoordinates", true);

    addResultInformation(information, graphs::ResultInformation_HandlerReturnType_GRAPH, "graph");
    addResultInformation(information, graphs::ResultInformation_HandlerReturnType_EDGE_COSTS,
                         "edgeCosts");
    addResultInformation(information,
                         graphs::ResultInformation_HandlerReturnType_VERTEX_COORDINATES,
                         "vertexCoordinates");
    return information;
}

kruskal_handler::kruskal_handler(std::unique_ptr<abstract_request> request)
{
    if (const auto *type_check_ptr = dynamic_cast<generic_request *>(request.get());
        !type_check_ptr)
    {
        throw server::request_parse_error("kruskal_handler: dynamic_cast failed!", request->type(),
                                          name());
    }
    m_request = std::unique_ptr<generic_request>{static_cast<generic_request *>(request.release())};
}

std::pair<graphs::ResponseContainer, long> kruskal_handler::handle()
{
    const graph_message *graph_message = m_request->graph_message();
    const ogdf::Graph &og_graph = graph_message->graph();
    const ogdf::EdgeArray<double> &og_weights = *m_request->edge_costs();
    const auto &og_edge_uids = graph_message->edge_uids();
    const auto &og_node_uids = graph_message->node_uids();
    const auto &og_node_coords = *m_request->node_coords();

    auto mst_graph = std::make_unique<ogdf::GraphCopySimple>(og_graph);
    auto mst_edge_uids = std::make_unique<ogdf::EdgeArray<server::uid_t>>(*mst_graph);
    ogdf::EdgeArray<double> mst_weights(*mst_graph);
    auto mst_node_uids = std::make_unique<ogdf::NodeArray<uid_t>>(*mst_graph);
    ogdf::NodeArray<node_coordinates> mst_node_coords(*mst_graph);

    for (ogdf::edge e : og_graph.edges)
    {
        mst_weights[mst_graph->copy(e)] = og_weights[e];
        mst_edge_uids->operator[](mst_graph->copy(e)) = og_edge_uids[e];
    }

    for (ogdf::node v : og_graph.nodes)
    {
        mst_node_uids->operator[](mst_graph->copy(v)) = og_node_uids[v];
        mst_node_coords[mst_graph->copy(v)] = og_node_coords[v];
    }

    auto start = std::chrono::high_resolution_clock::now();

    ogdf::makeMinimumSpanningTree<double>(*mst_graph, mst_weights);

    auto stop = std::chrono::high_resolution_clock::now();
    long ogdf_time = (std::chrono::duration_cast<std::chrono::microseconds>(stop - start)).count();

    server::generic_response::attribute_map<std::string> graph_attributes;

    server::graph_message mst_graph_message{std::move(mst_graph), std::move(mst_node_uids),
                                            std::move(mst_edge_uids)};

    return std::make_pair(
        response_factory::build_response(std::unique_ptr<abstract_response>{new generic_response{
            &mst_graph_message, &mst_node_coords, &mst_weights, nullptr, nullptr, nullptr, nullptr,
            nullptr, &graph_attributes, status_code::OK}}),
        ogdf_time);
}
}  // namespace server