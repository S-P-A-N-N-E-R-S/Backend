#include "handling/handlers/fault_tolerance_spanner/handler_resilient.hpp"
#include <chrono>
#include <typeinfo>
#include "networking/exceptions.hpp"
#include "networking/responses/generic_response.hpp"

namespace server {
resilient_handler::resilient_handler(std::unique_ptr<abstract_request> request)
    : m_request{}
{
    if (const auto *type_check_ptr = dynamic_cast<generic_request *>(request.get());
        !type_check_ptr)
    {
        throw server::request_parse_error("resilient_handler: dynamic_cast failed!",
                                          request->type(), name());
    }

    m_request = std::unique_ptr<generic_request>{static_cast<generic_request *>(request.release())};
}

handle_return resilient_handler::handle()
{
    const auto *graph_message = m_request->graph_message();
    auto &graph = graph_message->graph();

    ogdf::GraphAttributes ga(graph, ogdf::GraphAttributes::edgeDoubleWeight);
    ga.directed() = false;

    const auto *parsed_coords = m_request->node_coords();
    const auto *parsed_costs = m_request->edge_costs();

    for (auto edge_iterator = parsed_costs->begin(); edge_iterator != parsed_costs->end();
         edge_iterator++)
    {
        ga.doubleWeight(edge_iterator.key()) = edge_iterator.value();
    }

    double stretch = std::stod(m_request->graph_attributes().at("stretch"));
    int sigma = std::stoi(m_request->graph_attributes().at("sigma"));
    int internal_index = std::stoi(m_request->graph_attributes().at("internal"));

    ogdf::EdgeArray<bool> input_in_spanner(graph);

    auto spanner = std::make_unique<ogdf::GraphCopySimple>(graph);

    // TODO: internal_index not used yet
    ogdf::SpannerBasicGreedy<double> basic_greedy_algorithm;
    if (std::string error; !basic_greedy_algorithm.preconditionsOk(ga, stretch, error))
    {
        throw std::runtime_error("Precondition for input spanner not ok: " + error);
    }
    basic_greedy_algorithm.call(ga, stretch, *spanner, input_in_spanner);

    ogdf::ResilientSpanner<double> resilient_algorithm;
    ogdf::EdgeArray<bool> in_spanner(*spanner);

    // Scheduler enforces time limit
    resilient_algorithm.setTimelimit(-1);
    resilient_algorithm.setSigma(sigma);

    if (std::string error; !resilient_algorithm.preconditionsOk(ga, stretch, error))
    {
        throw std::runtime_error("Precondition for spanner not ok: " + error);
    }

    auto start = std::chrono::high_resolution_clock::now();
    resilient_algorithm.call(ga, stretch, *spanner, in_spanner);
    auto stop = std::chrono::high_resolution_clock::now();
    long ogdf_time = (std::chrono::duration_cast<std::chrono::microseconds>(stop - start)).count();

    const auto &og_node_uids = graph_message->node_uids();
    auto spanner_node_uids = std::make_unique<ogdf::NodeArray<server::uid_t>>(*spanner);
    auto spanner_node_coords = ogdf::NodeArray<node_coordinates>(*spanner);

    for (const auto spanner_node : spanner->nodes)
    {
        auto og_node = spanner->original(spanner_node);
        spanner_node_uids->operator[](spanner_node) = og_node_uids[og_node];
        spanner_node_coords[spanner_node] = parsed_coords->operator[](og_node);
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

    return {std::unique_ptr<abstract_response>{new generic_response{
                &spanner_gm, &spanner_node_coords, &spanner_edge_costs, nullptr, nullptr, nullptr,
                nullptr, nullptr, nullptr, status_code::OK}},
            ogdf_time};
}

graphs::HandlerInformation resilient_handler::handler_information()
{
    // add simple information
    auto information = createHandlerInformation(name(), graphs::RequestType::GENERIC);

    // add field information
    addFieldInformation(information, graphs::FieldInformation_FieldType_GRAPH, "Graph", "graph",
                        true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_DOUBLE, "Stretch factor",
                        "graphAttributes.stretch", true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_INT, "Sigma",
                        "graphAttributes.sigma", true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_INT,
                        "Internal Algorithm Index", "graphAttributes.internal", true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_EDGE_COSTS, "Edge costs",
                        "edgeCosts", true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_VERTEX_COORDINATES, "",
                        "vertexCoordinates", true);

    // add result information
    addResultInformation(information, graphs::ResultInformation_HandlerReturnType_GRAPH, "graph");
    addResultInformation(information, graphs::ResultInformation_HandlerReturnType_EDGE_COSTS,
                         "edgeCosts");
    addResultInformation(information,
                         graphs::ResultInformation_HandlerReturnType_VERTEX_COORDINATES,
                         "vertexCoordinates");

    return information;
}

std::string resilient_handler::name()
{
    return "Resilient";
}

}  // namespace server