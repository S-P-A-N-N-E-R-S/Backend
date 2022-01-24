#include <handling/handlers/girth_handler.hpp>

namespace server {

std::string girth_handler::name()
{
    return "Girth";
}

graphs::HandlerInformation girth_handler::handler_information()
{
    auto information = createHandlerInformation(name(), graphs::RequestType::GENERIC);

    addFieldInformation(information, graphs::FieldInformation_FieldType_GRAPH, "Graph", "graph",
                        true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_EDGE_COSTS, "Edge costs",
                        "edgeCosts", true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_VERTEX_COORDINATES, "",
                        "vertexCoordinates", true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_INT, "Unit weights",
                        "graphAttributes.unitWeights", true);

    addResultInformation(information, graphs::ResultInformation_HandlerReturnType_DOUBLE, "girth",
                         "girth");

    return information;
}

girth_handler::girth_handler(std::unique_ptr<abstract_request> request)
{
    if (const auto *type_check_ptr = dynamic_cast<generic_request *>(request.get());
        !type_check_ptr)
    {
        throw server::request_parse_error("girth_handler: dynamic_cast failed!", request->type(),
                                          name());
    }
    m_request = std::unique_ptr<generic_request>{static_cast<generic_request *>(request.release())};
}

handle_return girth_handler::handle()
{
    const graph_message *graph_message = m_request->graph_message();
    const ogdf::Graph &og_graph = graph_message->graph();
    const ogdf::EdgeArray<double> &og_weights = *m_request->edge_costs();
    bool unitWeights = (bool)std::stod(m_request->graph_attributes().at("unitWeights"));

    ogdf::EdgeArray<double> og_weights_copy;
    if (unitWeights)
    {
        og_weights_copy.init(og_graph, 1.0);
    }
    else
    {
        og_weights_copy.init(og_graph);
        for (ogdf::edge e : og_graph.edges)
        {
            og_weights_copy[e] = og_weights[e];
        }
    }

    auto start = std::chrono::high_resolution_clock::now();

    ogdf::GraphProperties gp;
    double girth = gp.girth(og_graph, og_weights_copy);

    auto stop = std::chrono::high_resolution_clock::now();
    long ogdf_time = (std::chrono::duration_cast<std::chrono::microseconds>(stop - start)).count();

    server::generic_response::attribute_map<std::string> graph_attributes;
    graph_attributes["girth"] = std::to_string(girth);

    return {std::unique_ptr<abstract_response>{
                new generic_response{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                     nullptr, &graph_attributes, status_code::OK}},
            ogdf_time};
}
}  // namespace server
