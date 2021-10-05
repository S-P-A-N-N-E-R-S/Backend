#include "handling/handlers/greedy_spanner_handler.hpp"

#include <chrono>
#include "networking/exceptions.hpp"
#include "networking/responses/generic_response.hpp"
#include "networking/responses/response_factory.hpp"

#include <ogdf/graphalg/SpannerBasicGreedy.h>

namespace server {

greedy_spanner_handler::greedy_spanner_handler(std::unique_ptr<abstract_request> request)
    : m_request{}
{
    if (const auto *type_check_ptr = dynamic_cast<generic_request *>(request.get());
        !type_check_ptr)
    {
        throw server::request_parse_error("greedy_spanner_handler: dynamic_cast failed!",
                                          request->type(), "greedy_spanner");
    }

    m_request = std::unique_ptr<generic_request>{static_cast<generic_request *>(request.release())};
}

std::pair<graphs::ResponseContainer, long> greedy_spanner_handler::handle()
{
    const auto *graph_message = m_request->graph_message();
    auto &graph = graph_message->graph();

    ogdf::GraphAttributes ga(graph, ogdf::GraphAttributes::edgeDoubleWeight);
    ga.directed() = false;

    const auto *parsed_costs = m_request->edge_costs();

    for (auto edge_iterator = parsed_costs->begin(); edge_iterator != parsed_costs->end();
         edge_iterator++)
    {
        ga.doubleWeight(edge_iterator.key()) = edge_iterator.value();
    }

    double stretch = std::stod(m_request->graph_attributes().at("stretch"));

    ogdf::EdgeArray<bool> in_spanner(graph);

    auto spanner = std::make_unique<ogdf::GraphCopySimple>(graph);

    ogdf::SpannerBasicGreedy<double> greedy_spanner_algorithm;
    //Scheduler enforces time limit
    greedy_spanner_algorithm.setTimelimit(-1);

    if (std::string error; !greedy_spanner_algorithm.preconditionsOk(ga, stretch, error))
    {
        throw std::runtime_error("Precondition for spanner not ok: " + error);
    }

    auto start = std::chrono::high_resolution_clock::now();

    auto return_type = greedy_spanner_algorithm.call(ga, stretch, *spanner, in_spanner);

    auto stop = std::chrono::high_resolution_clock::now();
    long ogdf_time = (std::chrono::duration_cast<std::chrono::microseconds>(stop - start)).count();

    const auto &og_node_uids = graph_message->node_uids();
    const auto *og_node_coords = m_request->node_coords();
    auto spanner_node_uids = std::make_unique<ogdf::NodeArray<server::uid_t>>(*spanner);
    auto spanner_node_coords = ogdf::NodeArray<node_coordinates>(*spanner);

    for (const auto spanner_node : spanner->nodes)
    {
        auto og_node = spanner->original(spanner_node);
        spanner_node_uids->operator[](spanner_node) = og_node_uids[og_node];
        spanner_node_coords[spanner_node] = og_node_coords->operator[](og_node);
    }

    const auto &og_edge_uids = graph_message->edge_uids();
    auto spanner_edge_uids = std::make_unique<ogdf::EdgeArray<server::uid_t>>(*spanner);
    auto spanner_edge_costs = ogdf::EdgeArray<double>(*spanner);

    for (const auto spanner_edge : spanner->edges)
    {
        auto og_edge = spanner->original(spanner_edge);
        spanner_edge_uids->operator[](spanner_edge) = og_edge_uids[og_edge];
        spanner_edge_costs[spanner_edge] = parsed_costs->operator[](og_edge);
    }

    server::graph_message spanner_gm{std::move(spanner), std::move(spanner_node_uids),
                                     std::move(spanner_edge_uids)};

    return std::make_pair(
        response_factory::build_response(std::unique_ptr<abstract_response>{
            new generic_response{&spanner_gm, &spanner_node_coords, &spanner_edge_costs, nullptr,
                                 nullptr, nullptr, nullptr, nullptr, nullptr, status_code::OK}}),
        ogdf_time);
}

graphs::HandlerInformation greedy_spanner_handler::handler_information()
{
    // add simple information
    auto information = greedy_spanner_handler::createHandlerInformation(
        "greedy_spanner", graphs::RequestType::GENERIC);
    // add field information
    greedy_spanner_handler::addFieldInformation(
        information, graphs::FieldInformation_FieldType_GRAPH, "Graph", "graph", true);
    greedy_spanner_handler::addFieldInformation(information,
                                                graphs::FieldInformation_FieldType_DOUBLE,
                                                "Stretch factor", "graphAttributes.stretch", true);
    greedy_spanner_handler::addFieldInformation(information,
                                                graphs::FieldInformation_FieldType_EDGE_COSTS,
                                                "Edge costs", "edgeCosts", true);
    greedy_spanner_handler::addFieldInformation(
        information, graphs::FieldInformation_FieldType_VERTEX_COORDINATES, "", "vertexCoordinates",
        true);

    // add result information
    greedy_spanner_handler::addResultInformation(
        information, graphs::ResultInformation_HandlerReturnType_GRAPH, "graph");
    greedy_spanner_handler::addResultInformation(
        information, graphs::ResultInformation_HandlerReturnType_EDGE_COSTS, "edgeCosts");
    greedy_spanner_handler::addResultInformation(
        information, graphs::ResultInformation_HandlerReturnType_VERTEX_COORDINATES,
        "vertexCoordinates");
    return information;
}

}  // namespace server
