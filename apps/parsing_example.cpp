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

    ogdf::EdgeArray<double> parsed_costs(graph);
    for (const auto &edge : graph.edges)
    {
      parsed_costs[edge] =
          std::stod(spr->graph_message()->edge_attrs().at("cost")[edge]);
    }

    const auto &origin = spr->graph_message()->node(spr->start_index());
    ogdf::NodeArray<ogdf::edge> preds;
    ogdf::NodeArray<double> dist;
    ogdf::Dijkstra<double>().call(graph, parsed_costs, origin, preds, dist);

    const auto &target = spr->graph_message()->node(spr->end_index());

    std::cout << "Distance to target: " << dist[target] << "\n";

    // Build shortest path graph from origin to index
    auto spg = std::make_unique<ogdf::Graph>();
    auto sp_node_uids = std::make_unique<ogdf::NodeArray<server::uid_t>>(*spg);
    auto sp_edge_uids = std::make_unique<ogdf::EdgeArray<server::uid_t>>(*spg);

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

        ogdf::node inserted_target = spg->newNode();
        (*(sp_node_uids))[inserted_target] = og_target_uid;

        ogdf::node inserted_source = spg->newNode();
        (*(sp_node_uids))[inserted_source] = og_source_uid;

        ogdf::edge inserted_edge = spg->newEdge(inserted_source, inserted_target);
        (*(sp_edge_uids))[inserted_edge] = og_edge_uid;

        cur_target = cur_source;
    }

    std::cout << "nof nodes after building le epic graph: " << spg->numberOfNodes() << "\n";
    std::cout << "nof edges after building le epic graph: " << spg->numberOfEdges() << "\n";

    auto spgm = std::make_unique<server::graph_message>(
        std::move(spg), std::move(sp_node_uids), std::move(sp_edge_uids),
        std::make_unique<ogdf::NodeArray<server::coords_t>>(*spg),
        std::make_unique<server::GraphAttributeMap>(), std::make_unique<server::NodeAttributeMap>(),
        std::make_unique<server::EdgeAttributeMap>());

    auto *resp = new server::shortest_path_response(std::move(spgm), server::status_code::OK);

    auto abs_resp =
        std::unique_ptr<server::abstract_response>(dynamic_cast<server::abstract_response *>(resp));

    server::response_factory rspf;
    auto response_container = rspf.build_response(abs_resp);
    std::cout << response_container->status() << "\n";

    std::cout << "done\n";
}
