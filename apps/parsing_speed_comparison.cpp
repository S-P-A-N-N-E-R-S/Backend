#include <chrono>
#include <iostream>
#include <string>
#include <tuple>

#include <ogdf/basic/List.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/graph_generators.h>
#include <ogdf/fileformats/GraphIO.h>

#include <google/protobuf/repeated_field.h>

#include "shortest_path.pb.h"

struct mock_edge_uid {
    int uid;
    int in_vertex_uid;
    int out_vertex_uid;
};

struct mock_edge_idx {
    int uid;
    size_t in_vertex_idx;
    size_t out_vertex_idx;
};

template <class T>
class mock_vector
{
public:
    mock_vector()
        : m_vec()
    {
    }

    mock_vector(int size)
        : m_vec(size)
    {
    }

    auto pushBack(const T &x)
    {
        this->m_vec.push_back(x);
        return (--this->m_vec.end());
    }

    void clear()
    {
        this->m_vec.clear();
    }

    auto size() const
    {
        return this->m_vec.size();
    }

    T &operator[](int pos)
    {
        return this->m_vec[pos];
    }

    const T &operator[](int pos) const
    {
        return this->m_vec[pos];
    }

private:
    std::vector<T> m_vec;
};

int main(int argc, const char **argv)
{
    // Uncomment for version with randomly generated connected graph
    // const auto nof_nodes = std::stoi(argv[1]);
    // const auto nof_edges = std::stoi(argv[2]);
    // ogdf::Graph graph;
    // ogdf::randomSimpleConnectedGraph(graph, nof_nodes, nof_edges);

    const auto file_name = argv[1];
    const auto replications = std::stoi(argv[2]);
    std::ifstream file(file_name);
    ogdf::Graph graph;
    ogdf::GraphIO::readGraphML(graph, file);
    file.close();

    for (size_t cur_rep = 0; cur_rep < replications; ++cur_rep)
    {
        const auto start = std::chrono::steady_clock::now();

        // These would usually be associated with graphs::Vertex messages but I don't wanna build proto messages now
        google::protobuf::RepeatedField<uint64_t> node_uids;
        google::protobuf::Map<uint64_t, graphs::VertexCoordinates> node_coords;

        for (const auto &node : graph.nodes)
        {
            const auto uid = node->index();

            node_uids.Add(uid);
            node_coords[uid] = [] {
                auto coords = graphs::VertexCoordinates{};
                coords.set_x(ogdf::randomDouble(-50, 50));
                coords.set_y(ogdf::randomDouble(-50, 50));
                coords.set_z(ogdf::randomDouble(-50, 50));

                return coords;
            }();
        }

        google::protobuf::RepeatedField<mock_edge_uid> edges;
        google::protobuf::Map<uint64_t, double> edge_costs;

        for (const auto &edge : graph.edges)
        {
            const auto uid = edge->index();

            const auto in_vertex_uid = edge->source()->index();
            const auto out_vertex_uid = edge->target()->index();

            edges.Add(mock_edge_uid{uid, in_vertex_uid, out_vertex_uid});
            edge_costs[uid] = ogdf::randomDouble(0, 100);
        }

        ogdf::Graph built_graph;
        ogdf::EdgeArray<double> costs(built_graph);
        ogdf::NodeArray<std::tuple<double, double, double>> coords(built_graph);

        std::unordered_map<uint64_t, ogdf::node> uid_to_node;
        std::unordered_map<uint64_t, ogdf::edge> uid_to_edge;

        for (auto it = node_uids.begin(); it != node_uids.end(); ++it)
        {
            const auto uid = *it;

            const ogdf::node inserted = built_graph.newNode();
            uid_to_node.insert({uid, inserted});
        }

        for (auto it = edges.begin(); it != edges.end(); ++it)
        {
            const auto uid = it->uid;
            const auto in_uid = it->in_vertex_uid;
            const auto out_uid = it->out_vertex_uid;

            const ogdf::node source = uid_to_node[in_uid];
            const ogdf::node target = uid_to_node[out_uid];

            const ogdf::edge inserted = built_graph.newEdge(source, target);
            uid_to_edge.insert({uid, inserted});
        }

        for (const auto &[uid, cost] : edge_costs)
        {
            const auto &edge = uid_to_edge[uid];
            costs[edge] = cost;
        }

        for (const auto &[uid, coord] : node_coords)
        {
            const auto &node = uid_to_node[uid];
            coords[node] = std::make_tuple(coord.x(), coord.y(), coord.z());
        }

        const auto end = std::chrono::steady_clock::now();
        std::cout << "UID-based: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                  << "\n";
        assert(built_graph.numberOfNodes() == graph.numberOfNodes());
        assert(built_graph.numberOfEdges() == graph.numberOfEdges());
    }

    for (size_t cur_rep = 0; cur_rep < replications; ++cur_rep)
    {
        const auto start = std::chrono::steady_clock::now();

        // These would usually be associated with graphs::Vertex messages but I don't wanna build proto messages now
        google::protobuf::RepeatedField<uint64_t> node_uids;
        google::protobuf::RepeatedField<graphs::VertexCoordinates> node_coords;

        // XXX: Kann man verhindern, hier mappen zu müssen?
        std::unordered_map<int, size_t> node_uids_to_index;

        size_t idx{0};
        for (const auto &node : graph.nodes)
        {
            const auto uid = node->index();

            node_uids.Add(uid);
            node_uids_to_index[uid] = idx++;
            node_coords.Add([] {
                auto coords = graphs::VertexCoordinates{};
                coords.set_x(ogdf::randomDouble(-50, 50));
                coords.set_y(ogdf::randomDouble(-50, 50));
                coords.set_z(ogdf::randomDouble(-50, 50));

                return coords;
            }());
        }

        google::protobuf::RepeatedField<mock_edge_idx> edges;
        google::protobuf::RepeatedField<uint64_t> edge_uids;
        google::protobuf::RepeatedField<double> edge_costs;

        for (const auto &edge : graph.edges)
        {
            const auto uid = edge->index();

            // XXX: Kann man verhindern, hier die Map verwenden zu müssen?
            const auto in_vertex_idx = node_uids_to_index[edge->source()->index()];
            const auto out_vertex_idx = node_uids_to_index[edge->target()->index()];

            edges.Add(mock_edge_idx{uid, in_vertex_idx, out_vertex_idx});
            edge_costs.Add(ogdf::randomDouble(0, 100));
        }

        ogdf::Graph built_graph;

        ogdf::NodeArray<uint64_t> node_uids_array(built_graph);
        ogdf::NodeArray<std::tuple<double, double, double>> coords(built_graph);

        ogdf::EdgeArray<uint64_t> edge_uids_array(built_graph);
        ogdf::EdgeArray<double> costs(built_graph);

        for (auto it = node_uids.begin(); it != node_uids.end(); ++it)
        {
            const auto uid = *it;

            const ogdf::node inserted = built_graph.newNode();
            node_uids_array[inserted] = uid;
        }

        ogdf::Array<ogdf::node> all_nodes(built_graph.numberOfNodes());
        built_graph.allNodes(all_nodes);

        for (auto it = edges.begin(); it != edges.end(); ++it)
        {
            const ogdf::node source = all_nodes[it->in_vertex_idx];
            const ogdf::node target = all_nodes[it->out_vertex_idx];

            const ogdf::edge inserted = built_graph.newEdge(source, target);
            edge_uids_array[inserted] = it->uid;
        }

        ogdf::Array<ogdf::edge> all_edges(built_graph.numberOfEdges());
        built_graph.allEdges(all_edges);

        for (int idx = 0; idx < edge_costs.size(); ++idx)
        {
            const auto &edge = all_edges[idx];
            costs[edge] = edge_costs[idx];
        }

        for (int idx = 0; idx < node_coords.size(); ++idx)
        {
            const auto &node = all_nodes[idx];
            const auto &coord = node_coords[idx];
            coords[node] = std::make_tuple(coord.x(), coord.y(), coord.z());
        }

        const auto end = std::chrono::steady_clock::now();

        std::cout << "Array-based: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                  << "\n";
        assert(built_graph.numberOfNodes() == graph.numberOfNodes());
        assert(built_graph.numberOfEdges() == graph.numberOfEdges());
    }

    for (size_t cur_rep = 0; cur_rep < replications; ++cur_rep)
    {
        const auto start = std::chrono::steady_clock::now();

        // These would usually be associated with graphs::Vertex messages but I don't wanna build proto messages now
        google::protobuf::RepeatedField<uint64_t> node_uids;
        google::protobuf::RepeatedField<graphs::VertexCoordinates> node_coords;

        // XXX: Kann man verhindern, hier mappen zu müssen?
        std::unordered_map<int, size_t> node_uids_to_index;

        size_t idx{0};
        for (const auto &node : graph.nodes)
        {
            const auto uid = node->index();

            node_uids.Add(uid);
            node_uids_to_index[uid] = idx++;
            node_coords.Add([] {
                auto coords = graphs::VertexCoordinates{};
                coords.set_x(ogdf::randomDouble(-50, 50));
                coords.set_y(ogdf::randomDouble(-50, 50));
                coords.set_z(ogdf::randomDouble(-50, 50));

                return coords;
            }());
        }

        google::protobuf::RepeatedField<mock_edge_idx> edges;
        google::protobuf::RepeatedField<uint64_t> edge_uids;
        google::protobuf::RepeatedField<double> edge_costs;

        for (const auto &edge : graph.edges)
        {
            const auto uid = edge->index();

            // XXX: Kann man verhindern, hier die Map verwenden zu müssen?
            const auto in_vertex_idx = node_uids_to_index[edge->source()->index()];
            const auto out_vertex_idx = node_uids_to_index[edge->target()->index()];

            edges.Add(mock_edge_idx{uid, in_vertex_idx, out_vertex_idx});
            edge_costs.Add(ogdf::randomDouble(0, 100));
        }

        ogdf::Graph built_graph;

        ogdf::NodeArray<uint64_t> node_uids_array(built_graph);
        ogdf::NodeArray<std::tuple<double, double, double>> coords(built_graph);

        ogdf::EdgeArray<uint64_t> edge_uids_array(built_graph);
        ogdf::EdgeArray<double> costs(built_graph);

        for (auto it = node_uids.begin(); it != node_uids.end(); ++it)
        {
            const auto uid = *it;

            const ogdf::node inserted = built_graph.newNode();
            node_uids_array[inserted] = uid;
        }

        mock_vector<ogdf::node> all_nodes(built_graph.numberOfNodes());
        built_graph.allNodes(all_nodes);

        for (auto it = edges.begin(); it != edges.end(); ++it)
        {
            const ogdf::node source = all_nodes[it->in_vertex_idx];
            const ogdf::node target = all_nodes[it->out_vertex_idx];

            const ogdf::edge inserted = built_graph.newEdge(source, target);
            edge_uids_array[inserted] = it->uid;
        }

        mock_vector<ogdf::edge> all_edges(built_graph.numberOfEdges());
        built_graph.allEdges(all_edges);

        for (int idx = 0; idx < edge_costs.size(); ++idx)
        {
            const auto &edge = all_edges[idx];
            costs[edge] = edge_costs[idx];
        }

        for (int idx = 0; idx < node_coords.size(); ++idx)
        {
            const auto &node = all_nodes[idx];
            const auto &coord = node_coords[idx];
            coords[node] = std::make_tuple(coord.x(), coord.y(), coord.z());
        }

        const auto end = std::chrono::steady_clock::now();

        std::cout << "Vector-based: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                  << "\n";
        assert(built_graph.numberOfNodes() == graph.numberOfNodes());
        assert(built_graph.numberOfEdges() == graph.numberOfEdges());
    }
}
