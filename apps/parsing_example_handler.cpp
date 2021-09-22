#include <iostream>
#include <string>

#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/graph_generators.h>
#include <ogdf/graphalg/Dijkstra.h>
#include "generic_container.pb.h"

#include <networking/messages/meta_data.hpp>
#include <networking/requests/request_factory.hpp>
#include <networking/responses/response_factory.hpp>

#include <handling/handler_proxy.hpp>

#include "meta.pb.h"

/**
 * @brief Most of this is copied from parsing_example, so kudos to whoever wrote
 * that one. Only difference is, we are using handlers
 **/
int main(int argc, const char **argv)
{
    graphs::GenericRequest proto_request;

    auto og = std::make_unique<ogdf::Graph>();
    ogdf::randomSimpleConnectedGraph(*og, 100, 300);

    auto node_uids = std::make_unique<ogdf::NodeArray<server::uid_t>>(*og);
    auto *node_coords = proto_request.mutable_vertexcoordinates();

    for (const auto &node : og->nodes)
    {
        const auto uid = node->index();

        (*node_uids)[node] = uid;
        node_coords->Add([] {
            auto coords = graphs::VertexCoordinates{};
            coords.set_x(ogdf::randomDouble(-50, 50));
            coords.set_y(ogdf::randomDouble(-50, 50));
            coords.set_z(ogdf::randomDouble(-50, 50));

            return coords;
        }());
    }

    auto edge_uids = std::make_unique<ogdf::EdgeArray<server::uid_t>>(*og);
    auto *edge_costs = proto_request.mutable_edgecosts();

    for (const auto &edge : og->edges)
    {
        const auto uid = edge->index();

        (*edge_uids)[edge] = uid;
        edge_costs->Add(ogdf::randomDouble(0, 100));
    }

    auto proto_graph = std::make_unique<graphs::Graph>(
        server::graph_message(std::move(og), std::move(node_uids), std::move(edge_uids))
            .as_proto());

    proto_request.set_allocated_graph(proto_graph.release());

    // Hardcoded for testing purposes only
    proto_request.mutable_graphattributes()->operator[]("startUid") = "0";

    graphs::RequestContainer proto_request_container;
    proto_request_container.mutable_request()->PackFrom(proto_request);

    auto [response_container, time] = server::handler_proxy().handle(
        server::meta_data{graphs::RequestType::GENERIC, "dijkstra"}, proto_request_container);

    graphs::GenericResponse parsed_resp;
    const bool ok = response_container.response().UnpackTo(&parsed_resp);
    assert(("Couldn't parse GenericResponse from container", ok));

    const double path_cost =
        std::accumulate(parsed_resp.edgecosts().begin(), parsed_resp.edgecosts().end(), 0.0);

    std::cout << "parsed graph nof nodes: " << parsed_resp.graph().vertexlist_size() << "\n";
    std::cout << "cost of path: " << path_cost << "\n";

    return 0;
}
