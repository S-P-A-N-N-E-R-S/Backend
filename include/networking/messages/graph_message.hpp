#pragma once

#include <memory>
#include <unordered_map>

#include <ogdf/basic/Graph.h>

#include "graph.pb.h"

namespace server {

using uid_t = uint64_t;

class graph_message
{
public:
    graph_message();
    graph_message(const graph_message &other);
    graph_message(graph_message &&msg) = default;
    graph_message(const graphs::Graph &proto);

    /**
     * @brief Builds a graph message from an ogdf::Graph and other required components.
     */
    graph_message(const ogdf::Graph &graph, const ogdf::NodeArray<uid_t> &node_uids,
                  const ogdf::EdgeArray<uid_t> &edge_uids);

    /**
     * @brief Builds a graph message from an ogdf::Graph and other required components.
     *
     * The constructed graph_message instance takes ownership of the passed pointers.
     */
    graph_message(std::unique_ptr<ogdf::Graph> graph,
                  std::unique_ptr<ogdf::NodeArray<uid_t>> node_uids,
                  std::unique_ptr<ogdf::EdgeArray<uid_t>> edge_uids);

    ~graph_message() = default;

    graph_message &operator=(const graph_message &other);
    graph_message &operator=(graph_message &&other) = default;

    graphs::Graph as_proto() const;

    const ogdf::node &node(uid_t uid) const;
    const ogdf::edge &edge(uid_t uid) const;

    const uid_t &uid() const;
    const ogdf::NodeArray<uid_t> &node_uids() const;
    const ogdf::EdgeArray<uid_t> &edge_uids() const;

    const ogdf::Graph &graph() const;

    /**
     * @brief Use this to obtain a index-based "view" on the nodes in `m_graph`. Necessary to map
     *        node attributes to nodes.
     */
    const ogdf::Array<ogdf::node> &all_nodes() const;

    /**
     * @brief Use this to obtain a index-based "view" on the nodes in `m_graph`. Necessary to map
     *        node attributes to nodes.
     */
    const ogdf::Array<ogdf::edge> &all_edges() const;

private:
    uid_t make_uid() const;

    std::unique_ptr<ogdf::Graph> m_graph;

    // XXX: This is only okay because `graph_message::graph()` returns a const &. Otherwise,
    //      we would not be able to make sure that the array is synchronized with `m_graph`.
    //      We could consider adding some kind of invalidation when implementing methods such
    //      as `graph_message::take_graph()` or similar.
    std::unique_ptr<ogdf::Array<ogdf::node>> m_all_nodes;
    std::unique_ptr<ogdf::Array<ogdf::edge>> m_all_edges;

    // TODO: Talk about if/how this should change when copying/modifying/etc. graph_messages
    uid_t m_uid{};
    std::unique_ptr<ogdf::NodeArray<uid_t>> m_node_uids;
    std::unique_ptr<ogdf::EdgeArray<uid_t>> m_edge_uids;

    std::unique_ptr<std::unordered_map<uid_t, ogdf::node>> m_uid_to_node;
    std::unique_ptr<std::unordered_map<uid_t, ogdf::edge>> m_uid_to_edge;
};

}  // namespace server
