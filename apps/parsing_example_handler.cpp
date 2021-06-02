#include <iostream>
#include <string>

#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/graph_generators.h>
#include <ogdf/graphalg/Dijkstra.h>

#include <networking/requests/request_factory.hpp>
#include <networking/responses/response_factory.hpp>

#include <handling/handler_proxy.hpp>

/**
 * @brief Most of this is copied from parsing_example, so kudos to whoever wrote
 * that one. Only difference is, we are using handlers
 **/
int main(int argc, const char **argv)
{
    auto og = std::make_unique<ogdf::Graph>();
    ogdf::randomSimpleConnectedGraph(*og, 100, 300);

    auto ga = std::make_unique<server::GraphAttributeMap>();
    ga->emplace("type", "euclidean");

    auto node_uids = std::make_unique<ogdf::NodeArray<server::uid_t>>(*og);
    auto node_coords = std::make_unique<ogdf::NodeArray<server::coords_t>>(*og);
    auto na = std::make_unique<server::NodeAttributeMap>();
    {
        ogdf::NodeArray<std::string> name(*og);
        for (const auto &node : og->nodes)
        {
            name[node] = "Vertex " + std::to_string(node->index());
            (*node_uids)[node] = node->index();
            (*node_coords)[node] =
                std::make_tuple(ogdf::randomDouble(-50, 50), ogdf::randomDouble(-50, 50),
                                ogdf::randomDouble(-50, 50));
        }

        na->emplace("name", std::move(name));
    }

    auto edge_uids = std::make_unique<ogdf::EdgeArray<server::uid_t>>(*og);
    auto ea = std::make_unique<server::EdgeAttributeMap>();
    {
        ogdf::EdgeArray<std::string> cost(*og);
        for (const auto &edge : og->edges)
        {
            cost[edge] = std::to_string(ogdf::randomDouble(0, 100));
            (*edge_uids)[edge] = edge->index();
        }

        ea->emplace("cost", std::move(cost));
    }

    auto proto_graph =
        server::graph_message(std::move(og), std::move(node_uids), std::move(edge_uids),
                              std::move(node_coords), std::move(ga), std::move(na), std::move(ea))
            .as_proto();

    graphs::ShortPathRequest proto_request;
    proto_request.set_allocated_graph(proto_graph.release());
    // Hardcoded for testing purposes only
    uid_t start = 0;
    uid_t end = 99;
    proto_request.set_startindex(start);
    proto_request.set_endindex(end);

    graphs::RequestContainer proto_request_container;
    proto_request_container.set_type(graphs::RequestContainer_RequestType_SHORTEST_PATH);
    proto_request_container.mutable_request()->PackFrom(proto_request);

    server::request_factory factory;
    std::unique_ptr<server::abstract_request> request =
        factory.build_request(proto_request_container);

    auto response = server::HandlerProxy().handle(std::move(request));

    return 0;
}
