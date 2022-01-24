#include <handling/handlers/fragility_handler.hpp>

namespace server {

std::string fragility_handler::name()
{
    return "Fragility";
}

graphs::HandlerInformation fragility_handler::handler_information()
{
    auto information = createHandlerInformation(name(), graphs::RequestType::GENERIC);

    addFieldInformation(information, graphs::FieldInformation_FieldType_GRAPH, "Graph", "graph",
                        true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_EDGE_COSTS, "Edge costs",
                        "edgeCosts", true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_VERTEX_COORDINATES, "",
                        "vertexCoordinates", true);

    addResultInformation(information, graphs::ResultInformation_HandlerReturnType_DOUBLE,
                         "avgFragility", "avgFragility");

    addResultInformation(information, graphs::ResultInformation_HandlerReturnType_DOUBLE,
                         "minFragility", "minFragility");

    addResultInformation(information, graphs::ResultInformation_HandlerReturnType_DOUBLE,
                         "maxFragility", "maxFragility");
    return information;
}

fragility_handler::fragility_handler(std::unique_ptr<abstract_request> request)
{
    if (const auto *type_check_ptr = dynamic_cast<generic_request *>(request.get());
        !type_check_ptr)
    {
        throw server::request_parse_error("fragility_handler: dynamic_cast failed!",
                                          request->type(), name());
    }
    m_request = std::unique_ptr<generic_request>{static_cast<generic_request *>(request.release())};
}

handle_return fragility_handler::handle()
{
    const graph_message *graph_message = m_request->graph_message();
    const ogdf::Graph &og_graph = graph_message->graph();
    const ogdf::EdgeArray<double> &og_weights = *m_request->edge_costs();

    ogdf::EdgeArray<double> og_weights_copy(og_graph);
    for (ogdf::edge e : og_graph.edges)
    {
        og_weights_copy[e] = og_weights[e];
    }

    auto start = std::chrono::high_resolution_clock::now();

    ogdf::GraphProperties gp;
    std::tuple<double, double, double> graph_fragility = gp.fragility(og_graph, og_weights_copy);

    auto stop = std::chrono::high_resolution_clock::now();
    long ogdf_time = (std::chrono::duration_cast<std::chrono::microseconds>(stop - start)).count();

    server::generic_response::attribute_map<std::string> graph_attributes;
    graph_attributes["avgFragility"] = std::to_string(std::get<0>(graph_fragility));
    graph_attributes["maxFragility"] = std::to_string(std::get<1>(graph_fragility));
    graph_attributes["minFragility"] = std::to_string(std::get<2>(graph_fragility));

    return {std::unique_ptr<abstract_response>{
                new generic_response{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                     nullptr, &graph_attributes, status_code::OK}},
            ogdf_time};
}
}  // namespace server
