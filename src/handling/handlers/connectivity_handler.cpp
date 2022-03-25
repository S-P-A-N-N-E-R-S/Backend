#include <handling/handlers/connectivity_handler.hpp>

namespace server {

std::string connectivity_handler::name()
{
    return "Connectivity";
}

graphs::HandlerInformation connectivity_handler::handler_information()
{
    auto information = createHandlerInformation(name(), graphs::RequestType::GENERIC);

    addFieldInformation(information, graphs::FieldInformation_FieldType_GRAPH, "Graph", "graph",
                        true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_EDGE_COSTS, "Edge costs",
                        "edgeCosts", true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_VERTEX_COORDINATES, "",
                        "vertexCoordinates", true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_INT, "Node connectivity",
                        "graphAttributes.nodeConnectivity", true);
    addFieldInformation(information, graphs::FieldInformation_FieldType_INT, "Directed",
                        "graphAttributes.directed", true);

    addResultInformation(information, graphs::ResultInformation_HandlerReturnType_INT,
                         "connectivity");

    return information;
}

connectivity_handler::connectivity_handler(std::unique_ptr<abstract_request> request)
{
    if (const auto *type_check_ptr = dynamic_cast<generic_request *>(request.get());
        !type_check_ptr)
    {
        throw server::request_parse_error("connectivity_handler: dynamic_cast failed!",
                                          request->type(), name());
    }
    m_request = std::unique_ptr<generic_request>{static_cast<generic_request *>(request.release())};
}

handle_return connectivity_handler::handle()
{
    const graph_message *graph_message = m_request->graph_message();
    const ogdf::Graph &og_graph = graph_message->graph();
    bool nodeConnectivity = (m_request->graph_attributes().at("nodeConnectivity") != "0");
    bool directed = (m_request->graph_attributes().at("directed") != "0");

    auto start = std::chrono::high_resolution_clock::now();

    ogdf::ConnectivityTester conTest(nodeConnectivity, directed);
    ogdf::NodeArray<ogdf::NodeArray<int>> matrix(og_graph, ogdf::NodeArray<int>(og_graph));
    int conValue = conTest.computeConnectivity(og_graph, matrix);

    auto stop = std::chrono::high_resolution_clock::now();
    long ogdf_time = (std::chrono::duration_cast<std::chrono::microseconds>(stop - start)).count();

    server::generic_response::attribute_map<std::string> graph_attributes;
    graph_attributes["connectivity"] = std::to_string(conValue);

    return {std::unique_ptr<abstract_response>{
                new generic_response{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                     nullptr, &graph_attributes, status_code::OK}},
            ogdf_time};
}
}  // namespace server
