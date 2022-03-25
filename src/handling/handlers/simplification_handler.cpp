#include "handling/handlers/simplification_handler.hpp"

#include <chrono>
#include "networking/exceptions.hpp"
#include "networking/responses/generic_response.hpp"
#include "networking/responses/response_factory.hpp"

namespace server {

simplification_handler::simplification_handler(std::unique_ptr<abstract_request> request)
    : m_request{}
{
    if (const auto *type_check_ptr = dynamic_cast<generic_request *>(request.get());
        !type_check_ptr)
    {
        throw server::request_parse_error("simplification_handler: dynamic_cast failed!",
                                          request->type(), "simplification");
    }

    m_request = std::unique_ptr<generic_request>{static_cast<generic_request *>(request.release())};
}

handle_return simplification_handler::handle()
{
    const auto *graph_message = m_request->graph_message();
    auto &graph = graph_message->graph();

    const auto *og_edge_costs = m_request->edge_costs();
    const auto &og_edge_uids = graph_message->edge_uids();
    const auto &og_node_uids = graph_message->node_uids();
    const auto *og_node_coords = m_request->node_coords();

    ogdf::GraphAttributes ga(graph, ogdf::GraphAttributes::edgeDoubleWeight);

    for (auto edge_iterator = og_edge_costs->begin(); edge_iterator != og_edge_costs->end();
         edge_iterator++)
    {
        ga.doubleWeight(edge_iterator.key()) = edge_iterator.value();
    }

    auto start = std::chrono::high_resolution_clock::now();
    auto simple_graph = std::make_unique<ogdf::GraphSimplification>(ga);
    auto stop = std::chrono::high_resolution_clock::now();
    long ogdf_time = (std::chrono::duration_cast<std::chrono::microseconds>(stop - start)).count();

    ogdf::GraphAttributes ga_simple = simple_graph->getGraphAttributes();

    auto node_uids = std::make_unique<ogdf::NodeArray<uid_t>>(*simple_graph);
    auto edge_uids = std::make_unique<ogdf::EdgeArray<server::uid_t>>(*simple_graph);
    auto edge_costs = ogdf::EdgeArray<double>(*simple_graph);
    auto node_coords = ogdf::NodeArray<node_coordinates>(*simple_graph);

    for (const auto node : simple_graph->nodes)
    {
        ogdf::node og_node = simple_graph->original(node);
        node_uids->operator[](node) = og_node_uids[og_node];
        node_coords[node] = og_node_coords->operator[](og_node);
    }

    for (const auto edge : simple_graph->edges)
    {
        (*edge_uids)[edge] = edge->index();
        edge_costs[edge] = ga_simple.doubleWeight(edge);
    }

    server::graph_message gm{std::move(simple_graph), std::move(node_uids), std::move(edge_uids)};

    return {std::unique_ptr<abstract_response>{
                new generic_response{&gm, &node_coords, &edge_costs, nullptr, nullptr, nullptr,
                                     nullptr, nullptr, nullptr, status_code::OK}},
            ogdf_time};
}

graphs::HandlerInformation simplification_handler::handler_information()
{
    // add simple information
    auto information =
        simplification_handler::createHandlerInformation(name(), graphs::RequestType::GENERIC);
    // add field information
    simplification_handler::addFieldInformation(
        information, graphs::FieldInformation_FieldType_GRAPH, "Graph", "graph", true);
    simplification_handler::addFieldInformation(information,
                                                graphs::FieldInformation_FieldType_EDGE_COSTS,
                                                "Edge costs", "edgeCosts", true);
    simplification_handler::addFieldInformation(
        information, graphs::FieldInformation_FieldType_VERTEX_COORDINATES, "", "vertexCoordinates",
        true);

    // add result information
    simplification_handler::addResultInformation(
        information, graphs::ResultInformation_HandlerReturnType_GRAPH, "graph");
    simplification_handler::addResultInformation(
        information, graphs::ResultInformation_HandlerReturnType_EDGE_COSTS, "edgeCosts");
    simplification_handler::addResultInformation(
        information, graphs::ResultInformation_HandlerReturnType_VERTEX_COORDINATES,
        "vertexCoordinates");
    return information;
}

std::string simplification_handler::name()
{
    return "simplification";
}

}  // namespace server
