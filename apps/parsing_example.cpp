#include <iostream>
#include <string>

#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/graph_generators.h>
#include <ogdf/graphalg/Dijkstra.h>

#include <networking/messages/graph_message.hpp>
#include <networking/requests/request_factory.hpp>
#include <networking/requests/shortest_path_request.hpp>
#include <networking/responses/response_factory.hpp>
#include <networking/responses/shortest_path_response.hpp>

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
    auto *edge_costs = proto_request.mutable_edgecosts();

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

    auto *spr = dynamic_cast<server::shortest_path_request *>(request.get());

    auto &graph = spr->graph_message()->graph();

    std::cout << "request type: " << static_cast<int>(spr->type()) << "\n";
    std::cout << "start index: " << spr->start_index() << "\n";
    std::cout << "end index: " << spr->end_index() << "\n";
    std::cout << "graph nof nodes: " << graph.numberOfNodes() << "\n";

    const auto *parsed_node_coords = spr->node_coords();
    const auto *parsed_costs = spr->edge_costs();

    const auto &origin = spr->graph_message()->node(spr->start_index());
    ogdf::NodeArray<ogdf::edge> preds;
    ogdf::NodeArray<double> dist;
    ogdf::Dijkstra<double>().call(graph, *parsed_costs, origin, preds, dist);

    const auto &target = spr->graph_message()->node(spr->end_index());

    std::cout << "Distance to target: " << dist[target] << "\n";

    // Build shortest path graph from origin to index
    auto spg = std::make_unique<ogdf::Graph>();
    auto sp_node_uids = std::make_unique<ogdf::NodeArray<server::uid_t>>(*spg);
    auto sp_edge_uids = std::make_unique<ogdf::EdgeArray<server::uid_t>>(*spg);

    auto sp_edge_costs = std::make_unique<ogdf::EdgeArray<double>>(*spg);
    auto sp_node_coords = std::make_unique<ogdf::NodeArray<server::node_coordinates>>(*spg);

    const auto &og_node_uids = spr->graph_message()->node_uids();
    const auto &og_edge_uids = spr->graph_message()->edge_uids();
    ogdf::node cur_target = target;

    while (cur_target != origin)
    {
        const ogdf::edge &pred_edge = preds[cur_target];
        const ogdf::node &cur_source = pred_edge->opposite(cur_target);

        const server::uid_t og_target_uid = og_node_uids[cur_target];
        const server::uid_t og_source_uid = og_node_uids[cur_source];
        const server::uid_t og_edge_uid = og_edge_uids[pred_edge];
        const double og_edge_cost = parsed_costs->operator[](pred_edge);

        ogdf::node inserted_target = spg->newNode();
        (*(sp_node_uids))[inserted_target] = og_target_uid;
        (*(sp_node_coords))[inserted_target] = (*(parsed_node_coords))[cur_target];

        ogdf::node inserted_source = spg->newNode();
        (*(sp_node_uids))[inserted_source] = og_source_uid;
        (*(sp_node_coords))[inserted_source] = (*(parsed_node_coords))[cur_source];

        ogdf::edge inserted_edge = spg->newEdge(inserted_source, inserted_target);
        (*(sp_edge_uids))[inserted_edge] = og_edge_uid;
        (*(sp_edge_costs))[inserted_edge] = og_edge_cost;

        cur_target = cur_source;
    }

    std::cout << "nof nodes after building le epic graph: " << spg->numberOfNodes() << "\n";
    std::cout << "nof edges after building le epic graph: " << spg->numberOfEdges() << "\n";

    auto spgm = std::make_unique<server::graph_message>(std::move(spg), std::move(sp_node_uids),
                                                        std::move(sp_edge_uids));

    auto resp = std::make_unique<server::shortest_path_response>(
        std::move(spgm), std::move(sp_edge_costs), std::move(sp_node_coords),
        server::status_code::OK);

    auto abs_resp = std::unique_ptr<server::abstract_response>(
        dynamic_cast<server::abstract_response *>(resp.release()));

    server::response_factory rspf;
    auto response_container = rspf.build_response(abs_resp);
    std::cout << response_container->status() << "\n";

    std::cout << "done\n";
}
