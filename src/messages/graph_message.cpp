#include "networking/messages/graph_message.hpp"

#include <chrono>

namespace server {

graph_message::graph_message()
    : m_graph(std::make_unique<ogdf::Graph>())
    , m_uid{this->make_uid()}
    , m_uid_to_node(std::make_unique<std::unordered_map<uid_t, ogdf::node>>())
    , m_uid_to_edge(std::make_unique<std::unordered_map<uid_t, ogdf::edge>>())
{
}

graph_message::graph_message(const graph_message &other)
    : m_graph(std::make_unique<ogdf::Graph>())
    , m_uid(this->make_uid())
    , m_node_uids(std::make_unique<ogdf::NodeArray<uid_t>>(*(this->m_graph)))
    , m_edge_uids(std::make_unique<ogdf::EdgeArray<uid_t>>(*(this->m_graph)))
    , m_uid_to_node(std::make_unique<std::unordered_map<uid_t, ogdf::node>>())
    , m_uid_to_edge(std::make_unique<std::unordered_map<uid_t, ogdf::edge>>())
{
    std::unordered_map<ogdf::node, ogdf::node> og_to_copy_node;
    std::unordered_map<ogdf::edge, ogdf::edge> og_to_copy_edge;

    // Copy graph manually in order to be able to access the newly inserted nodes/edges
    for (const ogdf::node &og_node : other.graph().nodes)
    {
        const ogdf::node copy_node = this->m_graph->newNode();
        og_to_copy_node.insert({og_node, copy_node});
    }

    for (const ogdf::edge &og_edge : other.graph().edges)
    {
        const ogdf::node copy_source = og_to_copy_node.at(og_edge->source());
        const ogdf::node copy_target = og_to_copy_node.at(og_edge->target());
        const ogdf::edge copy_edge = this->m_graph->newEdge(copy_source, copy_target);

        og_to_copy_edge.insert({og_edge, copy_edge});
    }

    for (const ogdf::node &og_node : other.graph().nodes)
    {
        const ogdf::node &copy_node = og_to_copy_node.at(og_node);

        const uid_t uid = (*(other.m_node_uids))[og_node];
        (*(this->m_node_uids))[copy_node] = uid;
        this->m_uid_to_node->insert({uid, copy_node});
    }

    for (const ogdf::edge &og_edge : other.graph().edges)
    {
        const ogdf::edge &copy_edge = og_to_copy_edge.at(og_edge);

        const uid_t uid = (*(other.m_edge_uids))[og_edge];
        (*(this->m_edge_uids))[copy_edge] = uid;
        this->m_uid_to_edge->insert({uid, copy_edge});
    }
}

graph_message::graph_message(const graphs::Graph &proto)
    : m_graph(std::make_unique<ogdf::Graph>())
    , m_uid{proto.uid()}
    , m_node_uids(std::make_unique<ogdf::NodeArray<uid_t>>(*(this->m_graph)))
    , m_edge_uids(std::make_unique<ogdf::EdgeArray<uid_t>>(*(this->m_graph)))
    , m_uid_to_node(std::make_unique<std::unordered_map<uid_t, ogdf::node>>())
    , m_uid_to_edge(std::make_unique<std::unordered_map<uid_t, ogdf::edge>>())
{
    for (auto it = proto.vertexlist().begin(); it != proto.vertexlist().end(); ++it)
    {
        const auto uid = it->uid();

        const ogdf::node inserted = this->m_graph->newNode();
        this->m_uid_to_node->insert({uid, inserted});
    }

    for (auto it = proto.edgelist().begin(); it != proto.edgelist().end(); ++it)
    {
        const auto uid = it->uid();
        const auto in_index = it->invertexuid();
        const auto out_index = it->outvertexuid();

        const ogdf::node source = (*(this->m_uid_to_node))[in_index];
        const ogdf::node target = (*(this->m_uid_to_node))[out_index];

        const ogdf::edge inserted = this->m_graph->newEdge(source, target);
        this->m_uid_to_edge->insert({uid, inserted});
    }

    for (auto node_it = proto.vertexlist().begin(); node_it != proto.vertexlist().end(); ++node_it)
    {
        const auto uid = node_it->uid();
        const auto &node = (*(this->m_uid_to_node))[uid];
        (*(this->m_node_uids))[node] = uid;
    }

    for (auto edge_it = proto.edgelist().begin(); edge_it != proto.edgelist().end(); ++edge_it)
    {
        const auto uid = edge_it->uid();
        const auto &edge = (*(this->m_uid_to_edge))[uid];
        (*(this->m_edge_uids))[edge] = uid;
    }
}

graph_message::graph_message(const ogdf::Graph &graph, const ogdf::NodeArray<uid_t> &node_uids,
                             const ogdf::EdgeArray<uid_t> &edge_uids)
    : m_graph(std::make_unique<ogdf::Graph>())
    , m_uid{this->make_uid()}
    , m_node_uids(std::make_unique<ogdf::NodeArray<uid_t>>(*(this->m_graph)))
    , m_edge_uids(std::make_unique<ogdf::EdgeArray<uid_t>>(*(this->m_graph)))
    , m_uid_to_node(std::make_unique<std::unordered_map<uid_t, ogdf::node>>())
    , m_uid_to_edge(std::make_unique<std::unordered_map<uid_t, ogdf::edge>>())
{
    std::unordered_map<ogdf::node, ogdf::node> og_to_copy_node;
    std::unordered_map<ogdf::edge, ogdf::edge> og_to_copy_edge;

    // ----------------- OGDF graph

    // Copy graph manually in order to be able to access the newly inserted nodes/edges
    for (const ogdf::node &og_node : graph.nodes)
    {
        const ogdf::node copy_node = this->m_graph->newNode();
        og_to_copy_node.insert({og_node, copy_node});
    }

    for (const ogdf::edge &og_edge : graph.edges)
    {
        const ogdf::node copy_source = og_to_copy_node.at(og_edge->source());
        const ogdf::node copy_target = og_to_copy_node.at(og_edge->target());
        const ogdf::edge copy_edge = this->m_graph->newEdge(copy_source, copy_target);

        og_to_copy_edge.insert({og_edge, copy_edge});
    }

    // ----------------- Node UIDs
    for (const ogdf::node &og_node : graph.nodes)
    {
        const ogdf::node &copy_node = og_to_copy_node.at(og_node);
        const uid_t uid = node_uids[og_node];
        (*(this->m_node_uids))[copy_node] = uid;
        this->m_uid_to_node->insert({uid, copy_node});
    }

    // ----------------- Edge UIDs
    for (const ogdf::edge &og_edge : graph.edges)
    {
        const ogdf::edge &copy_edge = og_to_copy_edge.at(og_edge);
        const uid_t uid = edge_uids[og_edge];
        (*(this->m_edge_uids))[copy_edge] = uid;
        this->m_uid_to_edge->insert({uid, copy_edge});
    }
}

graph_message::graph_message(std::unique_ptr<ogdf::Graph> graph,
                             std::unique_ptr<ogdf::NodeArray<uid_t>> node_uids,
                             std::unique_ptr<ogdf::EdgeArray<uid_t>> edge_uids)
    : m_graph(std::move(graph))
    , m_uid{this->make_uid()}
    , m_node_uids(std::move(node_uids))
    , m_edge_uids(std::move(edge_uids))
    , m_uid_to_node(std::make_unique<std::unordered_map<uid_t, ogdf::node>>())
    , m_uid_to_edge(std::make_unique<std::unordered_map<uid_t, ogdf::edge>>())
{
    // Ensure we didn't get nullptrs
    if (!(this->m_graph && this->m_node_uids && this->m_edge_uids))
    {
        throw std::invalid_argument("You may not pass a nullptr to the graph_message constructor");
    }

    // Update mappings
    for (const ogdf::node &node : this->m_graph->nodes)
    {
        this->m_uid_to_node->insert({(*(this->m_node_uids))[node], node});
    }

    for (const ogdf::edge &edge : this->m_graph->edges)
    {
        this->m_uid_to_edge->insert({(*(this->m_edge_uids))[edge], edge});
    }
}

graph_message &graph_message::operator=(const graph_message &other)
{
    if (this == &other)
    {
        return *this;
    }

    // Make sure graph is empty before copying
    this->m_graph.reset(new ogdf::Graph);

    std::unordered_map<ogdf::node, ogdf::node> og_to_copy_node;
    std::unordered_map<ogdf::edge, ogdf::edge> og_to_copy_edge;

    // Copy graph manually in order to be able to access the newly inserted nodes/edges
    for (const ogdf::node &og_node : other.graph().nodes)
    {
        const ogdf::node copy_node = this->m_graph->newNode();
        og_to_copy_node.insert({og_node, copy_node});
    }

    for (const ogdf::edge &og_edge : other.graph().edges)
    {
        const ogdf::node copy_source = og_to_copy_node.at(og_edge->source());
        const ogdf::node copy_target = og_to_copy_node.at(og_edge->target());
        const ogdf::edge copy_edge = this->m_graph->newEdge(copy_source, copy_target);

        og_to_copy_edge.insert({og_edge, copy_edge});
    }

    // Make sure attribute maps are empty before copying contents
    this->m_uid_to_node->clear();
    this->m_uid_to_edge->clear();

    this->m_uid = this->make_uid();
    this->m_node_uids = std::make_unique<ogdf::NodeArray<uid_t>>(*(this->m_graph));
    this->m_edge_uids = std::make_unique<ogdf::EdgeArray<uid_t>>(*(this->m_graph));

    for (const ogdf::node &og_node : other.graph().nodes)
    {
        const ogdf::node &copy_node = og_to_copy_node.at(og_node);

        const uid_t uid = (*(other.m_node_uids))[og_node];
        (*(this->m_node_uids))[copy_node] = uid;
        this->m_uid_to_node->insert({uid, copy_node});
    }

    for (const ogdf::edge &og_edge : other.graph().edges)
    {
        const ogdf::edge &copy_edge = og_to_copy_edge.at(og_edge);

        const uid_t uid = other.edge_uids()[og_edge];
        (*(this->m_edge_uids))[copy_edge] = uid;
        this->m_uid_to_edge->insert({uid, copy_edge});
    }

    return *this;
}

std::unique_ptr<graphs::Graph> graph_message::as_proto() const
{
    auto retval = std::make_unique<graphs::Graph>();

    retval->set_uid(this->m_uid);

    // Add nodes
    const auto &node_uids = *(this->m_node_uids);
    for (const ogdf::node &node : this->m_graph->nodes)
    {
        graphs::Vertex *inserted = retval->add_vertexlist();
        inserted->set_uid(node_uids[node]);
    }

    // Add edges
    const auto &edge_uids = *(this->m_edge_uids);
    for (const ogdf::edge &edge : this->m_graph->edges)
    {
        graphs::Edge *inserted = retval->add_edgelist();
        inserted->set_uid(edge_uids[edge]);

        inserted->set_invertexuid(node_uids[edge->source()]);
        inserted->set_outvertexuid(node_uids[edge->target()]);
    }

    return retval;
}

const ogdf::node &graph_message::node(uid_t uid) const
{
    return this->m_uid_to_node->at(uid);
}

const ogdf::edge &graph_message::edge(uid_t uid) const
{
    return this->m_uid_to_edge->at(uid);
}

const uid_t &graph_message::uid() const
{
    return this->m_uid;
}

const ogdf::NodeArray<uid_t> &graph_message::node_uids() const
{
    return *(this->m_node_uids);
}

const ogdf::EdgeArray<uid_t> &graph_message::edge_uids() const
{
    return *(this->m_edge_uids);
}

const ogdf::Graph &graph_message::graph() const
{
    return *(this->m_graph);
}

uid_t graph_message::make_uid() const
{
    using namespace std::chrono;

    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

}  // namespace server
