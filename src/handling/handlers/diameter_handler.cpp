#include <handling/handlers/diameter_handler.hpp>

namespace server {

std::string diameter_handler::name()
{
    return "Diameter";
}

graphs::HandlerInformation diameter_handler::handler_information()
{
    auto information = createHandlerInformation(name(), graphs::RequestType::GENERIC);

    addFieldInformation(information, graphs::FieldInformation_FieldType_GRAPH, "Graph", "graph",
                        true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_EDGE_COSTS, "Edge costs",
                        "edgeCosts", true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_VERTEX_COORDINATES, "",
                        "vertexCoordinates", true);

    addResultInformation(information, graphs::ResultInformation_HandlerReturnType_DOUBLE,
                         "diameter");

    return information;
}

diameter_handler::diameter_handler(std::unique_ptr<abstract_request> request)
{
    if (const auto *type_check_ptr = dynamic_cast<generic_request *>(request.get());
        !type_check_ptr)
    {
        throw server::request_parse_error("diameter_handler: dynamic_cast failed!", request->type(),
                                          name());
    }
    m_request = std::unique_ptr<generic_request>{static_cast<generic_request *>(request.release())};
}

handle_return diameter_handler::handle()
{
    const graph_message *graph_message = m_request->graph_message();
    const ogdf::Graph &og_graph = graph_message->graph();
    const ogdf::EdgeArray<double> &og_weights = *m_request->edge_costs();

    auto start = std::chrono::high_resolution_clock::now();

    ogdf::GraphProperties gp;
    double diameter = gp.graphDiameter(og_graph, og_weights);

    auto stop = std::chrono::high_resolution_clock::now();
    long ogdf_time = (std::chrono::duration_cast<std::chrono::microseconds>(stop - start)).count();

    server::generic_response::attribute_map<std::string> graph_attributes;
    graph_attributes["diameter"] = std::to_string(diameter);

    return {std::unique_ptr<abstract_response>{
                new generic_response{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                     nullptr, &graph_attributes, status_code::OK}},
            ogdf_time};
}
}  // namespace server
