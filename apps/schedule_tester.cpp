#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/graph_generators.h>
#include <ogdf/graphalg/Dijkstra.h>
#include <boost/process.hpp>
#include <networking/messages/meta_data.hpp>
#include <persistence/database_wrapper.hpp>
#include <scheduler/scheduler.hpp>
#include <thread>

#include "generic_container.pb.h"

server::binary_data generate_random_dijkstra(unsigned int seed, int n, int m);

/**
 * @brief Tests some simple scheduling options. Some possible manipulations are commented out
 *
 * @return int
 */
int main()
{
    std::string connection_string = "host=localhost port=5432 user= spanner_user dbname=spanner_db "
                                    "password=pwd connect_timeout=10";
    server::database_wrapper db(connection_string);

    int n = 1000;
    int m = n * 25;

    for (int i = 0; i < 10; i++)
    {
        std::cout << "Tester: Generating " << i << std::endl;
        auto data = generate_random_dijkstra(11235, n, m);
        db.add_job(1, server::meta_data{graphs::RequestType::GENERIC, "dijkstra"}, data);
    }

    auto &scheduler = server::scheduler::instance();
    // scheduler.set_time_limit(1000);
    //scheduler.set_resource_limit(1<<16);

    //auto sched_thread = scheduler.start();
    scheduler.start();

    // sleep(10);
    // scheduler.set_time_limit(3000);
    // sleep(2);
    // scheduler.set_time_limit(1);

    // sleep(6);
    // scheduler.set_resource_limit(1<<16);

    //sleep(2);
    // scheduler.stop_scheduler(false);
    //scheduler.stop_scheduler(true);

    //sched_thread.join();

    int status_seconds = 5;

    sleep(status_seconds);

    auto states = db.get_status(1);

    std::cout << "States after " << status_seconds << " seconds:\n";

    for (auto &status : states)
    {
        std::cout << status.first << ": " << status.second << "\n";
    }
    std::cout << std::endl;

    while (scheduler.running())
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
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
