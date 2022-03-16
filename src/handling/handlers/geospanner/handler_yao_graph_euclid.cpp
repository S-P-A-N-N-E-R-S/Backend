#include "handling/handlers/geospanner/handler_yao_graph_euclid.hpp"
#include <chrono>
#include <typeinfo>
#include "networking/exceptions.hpp"
#include "networking/responses/generic_response.hpp"

namespace server {
yao_graph_euclid_handler::yao_graph_euclid_handler(std::unique_ptr<abstract_request> request)
    : m_request{}
{
    if (const auto *type_check_ptr = dynamic_cast<generic_request *>(request.get());
        !type_check_ptr)
    {
        throw server::request_parse_error("yao_graph_euclid_handler: dynamic_cast failed!",
                                          request->type(), name());
    }

    m_request = std::unique_ptr<generic_request>{static_cast<generic_request *>(request.release())};
}

handle_return yao_graph_euclid_handler::handle()
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

    ogdf::EdgeArray<bool> in_spanner(graph);

    auto spanner = std::make_unique<ogdf::GraphCopySimple>(graph);

    ogdf::SpannerYaoGraphEuclidian yao_graph_algorithm;

    //Scheduler enforces time limit
    yao_graph_algorithm.setTimelimit(-1);

    if (std::string error; !yao_graph_algorithm.preconditionsOk(ga, stretch, error))
    {
        throw std::runtime_error("Precondition for spanner not ok: " + error);
    }

    auto start = std::chrono::high_resolution_clock::now();
    yao_graph_algorithm.call(ga, stretch, *spanner, in_spanner);
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

    auto spanner_edge_uids = std::make_unique<ogdf::EdgeArray<server::uid_t>>(*spanner);
    uid_t i = 0;
    for (const auto spanner_edge : spanner->edges)
    {
        spanner_edge_uids->operator[](spanner_edge) = i++;
    }

    server::graph_message spanner_gm{std::move(spanner), std::move(spanner_node_uids),
                                     std::move(spanner_edge_uids)};

    return {std::unique_ptr<abstract_response>{new generic_response{
                &spanner_gm, &spanner_node_coords, &yao_graph_algorithm.weights(), nullptr, nullptr,
                nullptr, nullptr, nullptr, nullptr, status_code::OK}},
            ogdf_time};
}

graphs::HandlerInformation yao_graph_euclid_handler::handler_information()
{
    // add simple information
    auto information = createHandlerInformation(name(), graphs::RequestType::GENERIC);
    // add field information
    addFieldInformation(information, graphs::FieldInformation_FieldType_GRAPH, "Graph", "graph",
                        true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_DOUBLE, "Stretch factor",
                        "graphAttributes.stretch", true);
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

std::string yao_graph_euclid_handler::name()
{
    return "Yao Graph (Euclidean)";
}
}  // namespace server