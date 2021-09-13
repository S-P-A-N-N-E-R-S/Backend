#include "networking/messages/graph_message.hpp"

#include <chrono>

namespace server {

graph_message::graph_message()
    : m_graph(std::make_unique<ogdf::Graph>())
    , m_all_nodes(std::make_unique<ogdf::Array<ogdf::node>>())
    , m_all_edges(std::make_unique<ogdf::Array<ogdf::edge>>())
    , m_uid{this->make_uid()}
    , m_uid_to_node(std::make_unique<std::unordered_map<uid_t, ogdf::node>>())
    , m_uid_to_edge(std::make_unique<std::unordered_map<uid_t, ogdf::edge>>())
{
}

graph_message::graph_message(const graph_message &other)
    : m_graph(std::make_unique<ogdf::Graph>())
    , m_all_nodes(std::make_unique<ogdf::Array<ogdf::node>>())
    , m_all_edges(std::make_unique<ogdf::Array<ogdf::edge>>())
    , m_uid(this->make_uid())
    , m_node_uids(std::make_unique<ogdf::NodeArray<uid_t>>(*(this->m_graph)))
    , m_edge_uids(std::make_unique<ogdf::EdgeArray<uid_t>>(*(this->m_graph)))
    , m_uid_to_node(std::make_unique<std::unordered_map<uid_t, ogdf::node>>())
    , m_uid_to_edge(std::make_unique<std::unordered_map<uid_t, ogdf::edge>>())
{
    ogdf::NodeArray<ogdf::node> og_to_copy_node(other.graph());

    // Copy graph manually in order to be able to access the newly inserted nodes/edges
    for (const ogdf::node &og_node : other.graph().nodes)
    {
        const ogdf::node copy_node = this->m_graph->newNode();
        og_to_copy_node[og_node] = copy_node;

        const uid_t uid = (*(other.m_node_uids))[og_node];
        (*(this->m_node_uids))[copy_node] = uid;
        this->m_uid_to_node->insert({uid, copy_node});
    }

    this->m_graph->allNodes(*(this->m_all_nodes));

    for (const ogdf::edge &og_edge : other.graph().edges)
    {
        const ogdf::node copy_source = og_to_copy_node[og_edge->source()];
        const ogdf::node copy_target = og_to_copy_node[og_edge->target()];
        const ogdf::edge copy_edge = this->m_graph->newEdge(copy_source, copy_target);

        const uid_t uid = (*(other.m_edge_uids))[og_edge];
        (*(this->m_edge_uids))[copy_edge] = uid;
        this->m_uid_to_edge->insert({uid, copy_edge});
    }

    this->m_graph->allEdges(*(this->m_all_edges));
}

graph_message::graph_message(const graphs::Graph &proto)
    : m_graph(std::make_unique<ogdf::Graph>())
    , m_all_nodes(std::make_unique<ogdf::Array<ogdf::node>>())
    , m_all_edges(std::make_unique<ogdf::Array<ogdf::edge>>())
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
        this->m_node_uids->operator[](inserted) = uid;
    }

    // Copy node objects into array to get random access
    this->m_graph->allNodes(*(this->m_all_nodes));

    for (auto it = proto.edgelist().begin(); it != proto.edgelist().end(); ++it)
    {
        const auto uid = it->uid();
        const auto in_index = it->invertexindex();
        const auto out_index = it->outvertexindex();

        const auto source = this->m_all_nodes->operator[](in_index);
        const auto target = this->m_all_nodes->operator[](out_index);

        const auto inserted = this->m_graph->newEdge(source, target);
        this->m_uid_to_edge->insert({uid, inserted});
        this->m_edge_uids->operator[](inserted) = uid;
    }

    this->m_graph->allEdges(*(this->m_all_edges));
}

graph_message::graph_message(const ogdf::Graph &graph, const ogdf::NodeArray<uid_t> &node_uids,
                             const ogdf::EdgeArray<uid_t> &edge_uids)
    : m_graph(std::make_unique<ogdf::Graph>())
    , m_all_nodes(std::make_unique<ogdf::Array<ogdf::node>>())
    , m_all_edges(std::make_unique<ogdf::Array<ogdf::edge>>())
    , m_uid{this->make_uid()}
    , m_node_uids(std::make_unique<ogdf::NodeArray<uid_t>>(*(this->m_graph)))
    , m_edge_uids(std::make_unique<ogdf::EdgeArray<uid_t>>(*(this->m_graph)))
    , m_uid_to_node(std::make_unique<std::unordered_map<uid_t, ogdf::node>>())
    , m_uid_to_edge(std::make_unique<std::unordered_map<uid_t, ogdf::edge>>())
{
    ogdf::NodeArray<ogdf::node> og_to_copy_node(graph);

    // Copy graph manually in order to be able to access the newly inserted nodes/edges
    for (const ogdf::node &og_node : graph.nodes)
    {
        const ogdf::node copy_node = this->m_graph->newNode();
        og_to_copy_node[og_node] = copy_node;

        const uid_t uid = node_uids[og_node];
        (*(this->m_node_uids))[copy_node] = uid;
        this->m_uid_to_node->insert({uid, copy_node});
    }

    this->m_graph->allNodes(*(this->m_all_nodes));

    for (const ogdf::edge &og_edge : graph.edges)
    {
        const ogdf::node copy_source = og_to_copy_node[og_edge->source()];
        const ogdf::node copy_target = og_to_copy_node[og_edge->target()];
        const ogdf::edge copy_edge = this->m_graph->newEdge(copy_source, copy_target);

        const uid_t uid = edge_uids[og_edge];
        (*(this->m_edge_uids))[copy_edge] = uid;
        this->m_uid_to_edge->insert({uid, copy_edge});
    }

    this->m_graph->allEdges(*(this->m_all_edges));
}

graph_message::graph_message(std::unique_ptr<ogdf::Graph> graph,
                             std::unique_ptr<ogdf::NodeArray<uid_t>> node_uids,
                             std::unique_ptr<ogdf::EdgeArray<uid_t>> edge_uids)
    : m_graph(std::move(graph))
    , m_all_nodes(std::make_unique<ogdf::Array<ogdf::node>>())
    , m_all_edges(std::make_unique<ogdf::Array<ogdf::edge>>())
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
    this->m_graph->allNodes(*(this->m_all_nodes));
    this->m_graph->allEdges(*(this->m_all_edges));

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

    // Make sure graph and nodes are empty before copying
    this->m_graph = std::make_unique<ogdf::Graph>();
    this->m_all_nodes = std::make_unique<ogdf::Array<ogdf::node>>();
    this->m_all_edges = std::make_unique<ogdf::Array<ogdf::edge>>();

    ogdf::NodeArray<ogdf::node> og_to_copy_node(other.graph());

    // Copy graph manually in order to be able to access the newly inserted nodes/edges
    for (const ogdf::node &og_node : other.graph().nodes)
    {
        const ogdf::node copy_node = this->m_graph->newNode();
        og_to_copy_node[og_node] = copy_node;

        const uid_t uid = (*(other.m_node_uids))[og_node];
        (*(this->m_node_uids))[copy_node] = uid;
        this->m_uid_to_node->insert({uid, copy_node});
    }

    this->m_graph->allNodes(*(this->m_all_nodes));

    for (const ogdf::edge &og_edge : other.graph().edges)
    {
        const ogdf::node copy_source = og_to_copy_node[og_edge->source()];
        const ogdf::node copy_target = og_to_copy_node[og_edge->target()];
        const ogdf::edge copy_edge = this->m_graph->newEdge(copy_source, copy_target);

        const uid_t uid = other.edge_uids()[og_edge];
        (*(this->m_edge_uids))[copy_edge] = uid;
        this->m_uid_to_edge->insert({uid, copy_edge});
    }

    this->m_graph->allEdges(*(this->m_all_edges));

    // Make sure attribute maps are empty before copying contents
    this->m_uid_to_node->clear();
    this->m_uid_to_edge->clear();

    this->m_uid = this->make_uid();
    this->m_node_uids = std::make_unique<ogdf::NodeArray<uid_t>>(*(this->m_graph));
    this->m_edge_uids = std::make_unique<ogdf::EdgeArray<uid_t>>(*(this->m_graph));

    return *this;
}

graphs::Graph graph_message::as_proto() const
{
    graphs::Graph retval;
    retval.set_uid(this->m_uid);

    // TODO(leon): Cache this? (Maybe not a good idea)
    // Map nodes to indices for edge representation
    ogdf::NodeArray<int> node_to_index(*(this->m_graph));

    const auto &node_uids = *(this->m_node_uids);
    int cur_idx = 0;

    // Add nodes
    for (const ogdf::node &node : this->m_graph->nodes)
    {
        graphs::Vertex *inserted = retval.add_vertexlist();
        inserted->set_uid(node_uids[node]);
        node_to_index[node] = cur_idx++;
    }

    // Add edges
    const auto &edge_uids = *(this->m_edge_uids);
    for (const ogdf::edge &edge : this->m_graph->edges)
    {
        graphs::Edge *inserted = retval.add_edgelist();
        inserted->set_uid(edge_uids[edge]);

        inserted->set_invertexindex(node_to_index[edge->source()]);
        inserted->set_outvertexindex(node_to_index[edge->target()]);
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

const ogdf::Array<ogdf::node> &graph_message::all_nodes() const
{
    return *(this->m_all_nodes);
}

const ogdf::Array<ogdf::edge> &graph_message::all_edges() const
{
    return *(this->m_all_edges);
}

uid_t graph_message::make_uid() const
{
    using namespace std::chrono;

    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

}  // namespace server
