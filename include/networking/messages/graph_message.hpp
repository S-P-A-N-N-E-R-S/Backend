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
    /**
     * @brief Constructs an empty graph message (with an empty graph).
     */
    graph_message();

    /**
     * @brief Copy constructor.
     */
    graph_message(const graph_message &other);

    /**
     * @brief Move constructor.
     */
    graph_message(graph_message &&msg) = default;

    /**
     * @brief Builds a graph message from a protocol buffer `Graph` message.
     *
     * Using `graph_message` over the "raw" protocol buffer object allows for usage of the OGDF API
     * and easy work with graph object's UIDs or indices.
     *
     * @param proto Protocol buffer object to parse from
     */
    graph_message(const graphs::Graph &proto);

    /**
     * @brief Builds a graph message from an ogdf::Graph and other required components.
     *
     * @param graph     Graph to attach to the message
     * @param node_uids UIDs for each node of `graph`
     * @param edge_uids UIDs for each edge of `graph`
     */
    graph_message(const ogdf::Graph &graph, const ogdf::NodeArray<uid_t> &node_uids,
                  const ogdf::EdgeArray<uid_t> &edge_uids);

    /**
     * @brief Builds a graph message from an ogdf::Graph and other required components.
     *
     * The constructed graph_message instance takes ownership of the passed pointers.
     *
     * @param graph     Graph to attach to the message
     * @param node_uids UIDs for each node of `graph`
     * @param edge_uids UIDs for each edge of `graph`
     */
    graph_message(std::unique_ptr<ogdf::Graph> graph,
                  std::unique_ptr<ogdf::NodeArray<uid_t>> node_uids,
                  std::unique_ptr<ogdf::EdgeArray<uid_t>> edge_uids);

    ~graph_message() = default;

    graph_message &operator=(const graph_message &other);
    graph_message &operator=(graph_message &&other) = default;

    /**
     * @brief Builds a protocol buffer message representing this `graph_message`.
     *
     * @return Protocol buffer representation of this `graph_message`
     */
    graphs::Graph as_proto() const;

    /**
     * @brief Access a node of the graph by its UID.
     *
     * @param uid UID of the requested node
     *
     * @return Graph node with the UID `uid`
     */
    const ogdf::node &node(uid_t uid) const;

    /**
     * @brief Access an edge of the graph by its UID.
     *
     * @param uid UID of the requested edge
     *
     * @return Graph edge with the UID `uid`
     */
    const ogdf::edge &edge(uid_t uid) const;

    /**
     * @brief Returns the UID of this `graph_message`.
     *
     * @return UID of this `graph_message`
     */
    const uid_t &uid() const;

    /**
     * @brief Returns the array of node UIDS of this `graph_message`.
     *
     * @return Array of node UIDs
     */
    const ogdf::NodeArray<uid_t> &node_uids() const;

    /**
     * @brief Returns the array of edge UIDS of this `graph_message`.
     *
     * @return Array of edge UIDs
     */
    const ogdf::EdgeArray<uid_t> &edge_uids() const;

    /**
     * @brief Returns the underlying OGDF graph of this `graph_message`.
     *
     * @return OGDF graph representation
     */
    const ogdf::Graph &graph() const;

    /**
     * @brief Obtains an index-based "view" on the nodes in `m_graph`. Necessary to map node
     *        attributes to nodes.
     *
     * @return Array of all nodes in the underlying graph
     */
    const ogdf::Array<ogdf::node> &all_nodes() const;

    /**
     * @brief Obtains an index-based "view" on the nodes in `m_graph`. Necessary to map edge
     *        attributes to edges.
     *
     * @return Array of all edges in the underlying graph
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
