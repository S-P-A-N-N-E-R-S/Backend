#include "handling/handlers/fault_tolerance_spanner/handler_bodwin_dinitz.hpp"
#include <chrono>
#include <typeinfo>
#include "networking/exceptions.hpp"
#include "networking/responses/generic_response.hpp"

namespace server {
bodwin_dinitz_handler::bodwin_dinitz_handler(std::unique_ptr<abstract_request> request)
    : m_request{}
{
    if (const auto *type_check_ptr = dynamic_cast<generic_request *>(request.get());
        !type_check_ptr)
    {
        throw server::request_parse_error("bodwin_dinitz_handler: dynamic_cast failed!",
                                          request->type(), name());
    }

    m_request = std::unique_ptr<generic_request>{static_cast<generic_request *>(request.release())};
}

handle_return bodwin_dinitz_handler::handle()
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
    int ft = std::stoi(m_request->graph_attributes().at("fault_tolerance"));
    double tau = std::stod(m_request->graph_attributes().at("tau"));
    double c = std::stod(m_request->graph_attributes().at("c"));

    ogdf::EdgeArray<bool> in_spanner(graph);

    auto spanner = std::make_unique<ogdf::GraphCopySimple>(graph);

    ogdf::FaultTolerantSpannerBodwinDinitz<double> bodwin_dinitz_algorithm;

    // Scheduler enforces time limit
    bodwin_dinitz_algorithm.setTimelimit(-1);
    bodwin_dinitz_algorithm.setFaultSize(ft);
    bodwin_dinitz_algorithm.setTau(tau);
    bodwin_dinitz_algorithm.setC(c);

    if (std::string error; !bodwin_dinitz_algorithm.preconditionsOk(ga, stretch, error))
    {
        throw std::runtime_error("Precondition for spanner not ok: " + error);
    }

    auto start = std::chrono::high_resolution_clock::now();
    bodwin_dinitz_algorithm.call(ga, stretch, *spanner, in_spanner);
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
                nullptr, nullptr, nullptr, status_code::OK}}, ogdf_time};
}

graphs::HandlerInformation bodwin_dinitz_handler::handler_information()
{
    // add simple information
    auto information = createHandlerInformation(name(), graphs::RequestType::GENERIC);

    // add field information
    addFieldInformation(information, graphs::FieldInformation_FieldType_GRAPH, "Graph", "graph",
                        true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_DOUBLE, "Stretch factor",
                        "graphAttributes.stretch", true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_INT,
                        "Fault tolerance parameter", "graphAttributes.fault_tolerance", true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_DOUBLE, "Tau",
                        "graphAttributes.tau", true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_DOUBLE, "C",
                        "graphAttributes.c", true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_EDGE_COSTS,"Edge costs",
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

std::string bodwin_dinitz_handler::name()
{
    return "Bodwin Dinitz";
}

}   // namespace server
