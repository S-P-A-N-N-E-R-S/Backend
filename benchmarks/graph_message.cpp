#include "networking/messages/graph_message.hpp"

#include <benchmark/benchmark.h>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/graph_generators.h>

namespace {

server::graph_message build_mock_graph_message()
{
    ogdf::Graph g;
    {
        static constexpr const int NOF_NODES = 10000;
        static constexpr const int NOF_EDGES = 30000;
        ogdf::randomSimpleConnectedGraph(g, NOF_NODES, NOF_EDGES);
    }

    ogdf::NodeArray<server::uid_t> node_uids(g);
    {
        int node_idx = 0;
        for (const auto &n : g.nodes)
        {
            node_uids[n] = node_idx++;
        }
    }

    ogdf::EdgeArray<server::uid_t> edge_uids(g);
    {
        int edge_idx = 0;
        for (const auto &e : g.edges)
        {
            edge_uids[e] = edge_idx++;
        }
    }

    return server::graph_message(g, node_uids, edge_uids);
    }

}  // namespace

static void BM_graph_message_ConstructDefault(benchmark::State &state)
{
    for (auto _ : state)
    {
        server::graph_message empty;
    }
}
BENCHMARK(BM_graph_message_ConstructDefault);

static void BM_graph_message_ConstructCopy(benchmark::State &state)
{
    const auto original = build_mock_graph_message();

    for (auto _ : state)
    {
        server::graph_message copy{original};
    }
}
BENCHMARK(BM_graph_message_ConstructCopy);

static void BM_graph_message_ConstructFromProto(benchmark::State &state)
{
    const auto proto_graph = [] {
        static constexpr const int NOF_NODES = 10000;

        graphs::Graph g;
        g.set_uid(0);

        for (int vertex_idx = 0; vertex_idx < NOF_NODES; ++vertex_idx)
        {
            auto *v = g.add_vertexlist();
            v->set_uid(vertex_idx);
        }

        for (int edge_idx = 0; edge_idx < NOF_NODES - 1; ++edge_idx)
        {
            auto *e = g.add_edgelist();
            e->set_uid(edge_idx);
            e->set_invertexindex(edge_idx);
            e->set_outvertexindex(edge_idx + 1);
        }

        return g;
    }();

    for (auto _ : state)
    {
        server::graph_message gm{proto_graph};
    }
}
BENCHMARK(BM_graph_message_ConstructFromProto);

static void BM_graph_message_ConstructFromMembers(benchmark::State &state)
{
    // Set up required members
    ogdf::Graph g;
    {
        static constexpr const int NOF_NODES = 10000;
        static constexpr const int NOF_EDGES = 30000;
        ogdf::randomSimpleConnectedGraph(g, NOF_NODES, NOF_EDGES);
    }

    ogdf::NodeArray<server::uid_t> node_uids(g);
    {
        int node_idx = 0;
        for (const auto &n : g.nodes)
        {
            node_uids[n] = node_idx++;
        }
    }

    ogdf::EdgeArray<server::uid_t> edge_uids(g);
    {
        int edge_idx = 0;
        for (const auto &e : g.edges)
        {
            edge_uids[e] = edge_idx++;
        }
    }

    for (auto _ : state)
    {
        server::graph_message{g, node_uids, edge_uids};
    }
}
BENCHMARK(BM_graph_message_ConstructFromMembers);

static void BM_graph_message_ConstructFromUniquePtrMembers(benchmark::State &state)
{
    for (auto _ : state)
    {
        // Set up required members. Needs to be done on every benchmark repetition since the
        // constructor consumes the unique_ptrs.
        auto g = std::make_unique<ogdf::Graph>();
        {
            static constexpr const int NOF_NODES = 10000;
            static constexpr const int NOF_EDGES = 30000;
            ogdf::randomSimpleConnectedGraph(*g, NOF_NODES, NOF_EDGES);
        }

        auto node_uids = std::make_unique<ogdf::NodeArray<server::uid_t>>(*g);
        {
            int node_idx = 0;
            for (const auto &n : g->nodes)
            {
                (*node_uids)[n] = node_idx++;
            }
        }

        auto edge_uids = std::make_unique<ogdf::EdgeArray<server::uid_t>>(*g);
        {
            int edge_idx = 0;
            for (const auto &e : g->edges)
            {
                (*edge_uids)[e] = edge_idx++;
            }
        }

        server::graph_message{std::move(g), std::move(node_uids), std::move(edge_uids)};
    }
}
BENCHMARK(BM_graph_message_ConstructFromUniquePtrMembers);

static void BM_graph_message_AssignmentCopy(benchmark::State &state)
{
    const auto original = build_mock_graph_message();

    for (auto _ : state)
    {
        server::graph_message copy = original;
    }
}
BENCHMARK(BM_graph_message_AssignmentCopy);

static void BM_graph_message_ConvertToProto(benchmark::State &state)
{
    const auto graph = build_mock_graph_message();

    for (auto _ : state)
    {
        graph.as_proto();
    }
}
BENCHMARK(BM_graph_message_ConvertToProto);
