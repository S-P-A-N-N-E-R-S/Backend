#include <iostream>

#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/graph_generators.h>
#include <ogdf/graphalg/Dijkstra.h>

#include <handling/handler_proxy.hpp>
#include "networking/requests/abstract_request.hpp"
#include "persistence/database_wrapper.hpp"

typedef std::basic_string<std::byte> binary_data;

binary_data generateRandomDijkstra(unsigned int seed);

int main(int argc, const char **argv)
{
    std::string connection_string = "host=localhost port=5432 user= spanner_user dbname=spanner_db "
                                    "password=pwd connect_timeout=10";
    auto data = generateRandomDijkstra(123);
    std::cout << "add_job:" << std::endl;
    server::database_wrapper persistence(connection_string);
    persistence.add_job(1234, data);

    auto next_jobs = persistence.get_next_jobs(5);
    std::cout << "get_next_jobs:" << std::endl;
    for (auto p : next_jobs)
    {
        std::cout << p.first << " " << p.second << std::endl;
    }

    next_jobs = persistence.get_next_jobs(1);
    if (next_jobs.size() == 1)
    {
        std::cout << "3" << std::endl;
        auto request = persistence.get_request_data(next_jobs[0].first, next_jobs[0].second);

        std::cout << "4" << std::endl;
        persistence.set_started(next_jobs[0].first);

        // Use handler to calculate result
        long ogdf_time;
        auto response = server::HandlerProxy().handle(request);

        std::cout << "5" << std::endl;

        // pack result into db
        persistence.add_response(next_jobs[0].first, *(response.first), response.second);
    }
}

binary_data generateRandomDijkstra(unsigned int seed)
{
    // Build a dummy Dijkstra Request
    ogdf::setSeed(seed);

    graphs::ShortestPathRequest proto_request;

    auto og = std::make_unique<ogdf::Graph>();

    ogdf::randomSimpleConnectedGraph(*og, 100, 500);

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
    proto_request.set_startuid(0);
    proto_request.set_enduid(99);

    graphs::RequestContainer proto_request_container;
    proto_request_container.set_type(graphs::RequestType::SHORTEST_PATH);
    proto_request_container.mutable_request()->PackFrom(proto_request);

    // This is important: pqxx expects a basic_string<byte>, so we directly serialize it to this and not to char* as in connection.cpp
    binary_data binary;
    binary.resize(proto_request_container.ByteSizeLong());
    proto_request_container.SerializeToArray(binary.data(), binary.size());

    return binary;
}