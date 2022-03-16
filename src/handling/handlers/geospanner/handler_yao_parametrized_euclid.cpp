#include "handling/handlers/geospanner/handler_yao_parametrized_euclid.hpp"
#include <chrono>
#include <typeinfo>
#include "networking/exceptions.hpp"
#include "networking/responses/generic_response.hpp"

#include <ogdf/graphalg/SpannerBasicGreedy.h>

namespace server {
yao_parametrized_euclid_handler::yao_parametrized_euclid_handler(
    std::unique_ptr<abstract_request> request)
    : m_request{}
{
    if (const auto *type_check_ptr = dynamic_cast<generic_request *>(request.get());
        !type_check_ptr)
    {
        throw server::request_parse_error("yao_parametrized_euclid_handler: dynamic_cast failed!",
                                          request->type(), name());
    }

    m_request = std::unique_ptr<generic_request>{static_cast<generic_request *>(request.release())};
}

handle_return yao_parametrized_euclid_handler::handle()
{
    const auto *graph_message = m_request->graph_message();
    auto &graph = graph_message->graph();

    ogdf::GraphAttributes ga(graph);
    ga.directed() = false;

    const auto *parsed_coords = m_request->node_coords();

    for (auto node_iterator = parsed_coords->begin(); node_iterator != parsed_coords->end();
         node_iterator++)
    {
        ga.x(node_iterator.key()) = node_iterator.value().m_x;
        ga.y(node_iterator.key()) = node_iterator.value().m_y;
    }

    double stretch = std::stod(m_request->graph_attributes().at("stretch"));
    double stretch_yao = std::stod(m_request->graph_attributes().at("stretch_yao"));
    if (stretch_yao <= 1.0 || stretch_yao > stretch)
    {
        stretch_yao = sqrt(stretch_yao);
    }

    ogdf::EdgeArray<bool> in_spanner(graph);

    ogdf::GraphCopySimple spanner_yao(graph);

    ogdf::SpannerYaoGraphEuclidian yao_graph_algorithm;

    //Scheduler enforces time limit
    yao_graph_algorithm.setTimelimit(-1);

    if (std::string error; !yao_graph_algorithm.preconditionsOk(ga, stretch_yao, error))
    {
        throw std::runtime_error("Precondition for spanner not ok: " + error);
    }

    auto start = std::chrono::high_resolution_clock::now();
    yao_graph_algorithm.call(ga, stretch, spanner_yao, in_spanner);

    ogdf::GraphAttributes gy(spanner_yao, ogdf::GraphAttributes::edgeDoubleWeight);
    for (auto edge : spanner_yao.edges)
    {
        gy.doubleWeight(edge) = yao_graph_algorithm.weights()[edge];
    }

    auto spanner_final = std::make_unique<ogdf::GraphCopySimple>(graph);

    ogdf::SpannerBasicGreedy<double> greedy;
    greedy.setTimelimit(-1);

    in_spanner = ogdf::EdgeArray<bool>(*spanner_final);

    greedy.call(gy, stretch / stretch_yao, *spanner_final, in_spanner);

    auto stop = std::chrono::high_resolution_clock::now();
    long ogdf_time = (std::chrono::duration_cast<std::chrono::microseconds>(stop - start)).count();

    const auto &og_node_uids = graph_message->node_uids();
    auto spanner_node_uids = std::make_unique<ogdf::NodeArray<server::uid_t>>(*spanner_final);
    auto spanner_node_coords = ogdf::NodeArray<node_coordinates>(*spanner_final);

    for (const auto spanner_node : spanner_final->nodes)
    {
        auto og_node = spanner_yao.original(spanner_final->original(spanner_node));
        spanner_node_uids->operator[](spanner_node) = og_node_uids[og_node];
        spanner_node_coords[spanner_node] = parsed_coords->operator[](og_node);
    }

    ogdf::EdgeArray<double> spanner_edge_costs(*spanner_final);
    auto spanner_edge_uids = std::make_unique<ogdf::EdgeArray<server::uid_t>>(*spanner_final);
    uid_t i = 0;
    for (const auto spanner_edge : spanner_final->edges)
    {
        spanner_edge_costs[spanner_edge] =
            yao_graph_algorithm.weights()[spanner_final->original(spanner_edge)];
        spanner_edge_uids->operator[](spanner_edge) = i++;
    }

    server::graph_message spanner_gm{std::move(spanner_final), std::move(spanner_node_uids),
                                     std::move(spanner_edge_uids)};

    return {std::unique_ptr<abstract_response>{new generic_response{
                &spanner_gm, &spanner_node_coords, &spanner_edge_costs, nullptr, nullptr, nullptr,
                nullptr, nullptr, nullptr, status_code::OK}},
            ogdf_time};
}

graphs::HandlerInformation yao_parametrized_euclid_handler::handler_information()
{
    // add simple information
    auto information = createHandlerInformation(name(), graphs::RequestType::GENERIC);
    // add field information
    addFieldInformation(information, graphs::FieldInformation_FieldType_GRAPH, "Graph", "graph",
                        true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_DOUBLE, "Stretch factor",
                        "graphAttributes.stretch", true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_DOUBLE,
                        "Stretch factor yao", "graphAttributes.stretch_yao", true);
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

std::string yao_parametrized_euclid_handler::name()
{
    return "Yao Graph Parametrized Pruning (Euclidean)";
}
}  // namespace server