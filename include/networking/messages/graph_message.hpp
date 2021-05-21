#pragma once

#include <memory>
#include <unordered_map>

#include <ogdf/basic/Graph.h>

#include "GraphData.pb.h"

namespace server {

using uid_t = uint64_t;
using coords_t = std::tuple<double, double, double>;

using GraphAttributeMap = std::unordered_map<std::string, std::string>;
using NodeAttributeMap = std::unordered_map<std::string, ogdf::NodeArray<std::string>>;
using EdgeAttributeMap = std::unordered_map<std::string, ogdf::EdgeArray<std::string>>;

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
                  const ogdf::EdgeArray<uid_t> &edge_uids,
                  const ogdf::NodeArray<coords_t> &node_coords,
                  const GraphAttributeMap &graph_attrs, const NodeAttributeMap &node_attrs,
                  const EdgeAttributeMap &edge_attrs);

    /**
     * @brief Builds a graph message from an ogdf::Graph and other required components.
     *
     * The constructed graph_message instance takes ownership of the passed pointers.
     */
    graph_message(std::unique_ptr<ogdf::Graph> graph,
                  std::unique_ptr<ogdf::NodeArray<uid_t>> node_uids,
                  std::unique_ptr<ogdf::EdgeArray<uid_t>> edge_uids,
                  std::unique_ptr<ogdf::NodeArray<coords_t>> node_coords,
                  std::unique_ptr<GraphAttributeMap> graph_attrs,
                  std::unique_ptr<NodeAttributeMap> node_attrs,
                  std::unique_ptr<EdgeAttributeMap> edge_attrs);

    ~graph_message() = default;

    graph_message &operator=(const graph_message &other);
    graph_message &operator=(graph_message &&other) = default;

    std::unique_ptr<graphs::Graph> as_proto() const;

    const ogdf::node &node(uid_t uid) const;
    const ogdf::edge &edge(uid_t uid) const;

    const uid_t &uid() const;
    const ogdf::NodeArray<uid_t> &node_uids() const;
    const ogdf::EdgeArray<uid_t> &edge_uids() const;

    const ogdf::Graph &graph() const;
    const GraphAttributeMap &graph_attrs() const;
    const NodeAttributeMap &node_attrs() const;
    const EdgeAttributeMap &edge_attrs() const;

    const ogdf::NodeArray<coords_t> &node_coords() const;

private:
    uid_t make_uid() const;

    std::unique_ptr<ogdf::Graph> m_graph;

    // TODO: Talk about if/how this should change when copying/modifying/etc. graph_messages
    uid_t m_uid{};
    std::unique_ptr<ogdf::NodeArray<uid_t>> m_node_uids;
    std::unique_ptr<ogdf::EdgeArray<uid_t>> m_edge_uids;

    std::unique_ptr<std::unordered_map<uid_t, ogdf::node>> m_uid_to_node;
    std::unique_ptr<std::unordered_map<uid_t, ogdf::edge>> m_uid_to_edge;

    std::unique_ptr<GraphAttributeMap> m_graph_attrs;
    std::unique_ptr<NodeAttributeMap> m_node_attrs;
    std::unique_ptr<EdgeAttributeMap> m_edge_attrs;

    // TODO: Consider removing these, see protocol#2
    std::unique_ptr<ogdf::NodeArray<coords_t>> m_node_coords;
};

}  // namespace server
