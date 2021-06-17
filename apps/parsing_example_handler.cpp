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
    graphs::ShortestPathRequest proto_request;

    auto og = std::make_unique<ogdf::Graph>();
    ogdf::randomSimpleConnectedGraph(*og, 100, 300);

    auto node_uids = std::make_unique<ogdf::NodeArray<server::uid_t>>(*og);
    auto *node_coords = proto_request.mutable_vertexcoordinates();

    for (const auto &node : og->nodes)
    {
        const auto uid = node->index();

        (*node_uids)[node] = uid;
        node_coords->operator[](uid) = [] {
            auto coords = graphs::VertexCoordinates{};
            coords.set_x(ogdf::randomDouble(-50, 50));
            coords.set_y(ogdf::randomDouble(-50, 50));
            coords.set_z(ogdf::randomDouble(-50, 50));

            return coords;
        }();
    }

    auto edge_uids = std::make_unique<ogdf::EdgeArray<server::uid_t>>(*og);
    auto *edge_costs = proto_request.mutable_edgecost();

    for (const auto &edge : og->edges)
    {
        const auto uid = edge->index();

        (*edge_uids)[edge] = uid;
        edge_costs->operator[](uid) = ogdf::randomDouble(0, 100);
    }

    auto proto_graph =
        server::graph_message(std::move(og), std::move(node_uids), std::move(edge_uids)).as_proto();

    proto_request.set_allocated_graph(proto_graph.release());

    // Hardcoded for testing purposes only
    proto_request.set_startindex(0);
    proto_request.set_endindex(99);

    graphs::RequestContainer proto_request_container;
    proto_request_container.set_type(graphs::RequestContainer_RequestType_SHORTEST_PATH);
    proto_request_container.mutable_request()->PackFrom(proto_request);

    server::request_factory factory;
    std::unique_ptr<server::abstract_request> request =
        factory.build_request(proto_request_container);

    auto response = server::HandlerProxy().handle(std::move(request));

    return 0;
}
