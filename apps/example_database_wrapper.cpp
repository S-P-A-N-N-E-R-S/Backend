#include <iostream>

#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/graph_generators.h>
#include <ogdf/graphalg/Dijkstra.h>

#include <config/config.hpp>
#include <handling/handler_utilities.hpp>
#include <networking/messages/graph_message.hpp>
#include <networking/messages/meta_data.hpp>
#include <networking/requests/abstract_request.hpp>
#include <persistence/database_wrapper.hpp>

#include "generic_container.pb.h"

server::binary_data generate_random_dijkstra(unsigned int seed, int n, int m);

int main(int argc, const char **argv)
{
    // Parse configurations
    server::config_parser::instance().parse(argc, argv);

    std::string connection_string = server::get_db_connection_string();
    auto data = generate_random_dijkstra(123, 100, 1000);
    std::cout << "add_job:" << std::endl;
    server::database_wrapper persistence(connection_string);
    persistence.add_job(1234, server::meta_data{graphs::RequestType::GENERIC, "dijkstra"}, data);

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
        server::meta_data job_meta_data =
            persistence.get_meta_data(next_jobs[0].first, next_jobs[0].second);
        auto [type, request] =
            persistence.get_request_data(next_jobs[0].first, next_jobs[0].second);

        std::cout << "4" << std::endl;
        persistence.set_started(next_jobs[0].first);

        // Use handler to calculate result
        auto [_, ogdf_time, response] = server::handle(job_meta_data, request);

        std::cout << "5" << std::endl;

        // pack result into db
        persistence.add_response(next_jobs[0].first, type, response, ogdf_time);
    }
}

server::binary_data generate_random_dijkstra(unsigned int seed, int n, int m)
{
    // Build a dummy Dijkstra Request
    ogdf::setSeed(seed);

    graphs::GenericRequest proto_request;

    auto og = std::make_unique<ogdf::Graph>();

    ogdf::randomSimpleConnectedGraph(*og, n, m);

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
    (*proto_request.mutable_graphattributes())["startUid"] = "0";

    graphs::RequestContainer proto_request_container;
    proto_request_container.mutable_request()->PackFrom(proto_request);

    // This is important: pqxx expects a basic_string<byte>, so we directly serialize it to this and not to char* as in connection.cpp
    server::binary_data binary;
    binary.resize(proto_request_container.ByteSizeLong());
    proto_request_container.SerializeToArray(binary.data(), binary.size());

    return binary;
}
