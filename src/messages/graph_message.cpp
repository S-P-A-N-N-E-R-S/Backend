#include "networking/messages/graph_message.hpp"

#include <chrono>

namespace server {

graph_message::graph_message()
    : m_graph(std::make_unique<ogdf::Graph>())
    , m_uid{this->make_uid()}
    , m_uid_to_node(std::make_unique<std::unordered_map<uid_t, ogdf::node>>())
    , m_uid_to_edge(std::make_unique<std::unordered_map<uid_t, ogdf::edge>>())
    , m_graph_attrs(std::make_unique<GraphAttributeMap>())
    , m_node_attrs(std::make_unique<NodeAttributeMap>())
    , m_edge_attrs(std::make_unique<EdgeAttributeMap>())
    , m_node_coords(std::make_unique<ogdf::NodeArray<coords_t>>())
{
}

graph_message::graph_message(const graph_message &other)
    : m_graph(std::make_unique<ogdf::Graph>())
    , m_uid(this->make_uid())
    , m_node_uids(std::make_unique<ogdf::NodeArray<uid_t>>(*(this->m_graph)))
    , m_edge_uids(std::make_unique<ogdf::EdgeArray<uid_t>>(*(this->m_graph)))
    , m_uid_to_node(std::make_unique<std::unordered_map<uid_t, ogdf::node>>())
    , m_uid_to_edge(std::make_unique<std::unordered_map<uid_t, ogdf::edge>>())
    , m_graph_attrs(std::make_unique<GraphAttributeMap>(*(other.m_graph_attrs)))
    , m_node_attrs(std::make_unique<NodeAttributeMap>())
    , m_edge_attrs(std::make_unique<EdgeAttributeMap>())
    , m_node_coords(std::make_unique<ogdf::NodeArray<coords_t>>(*(this->m_graph)))
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

        (*(this->m_node_coords))[copy_node] = (*(other.m_node_coords))[og_node];
    }

    for (const ogdf::edge &og_edge : other.graph().edges)
    {
        const ogdf::edge &copy_edge = og_to_copy_edge.at(og_edge);

        const uid_t uid = (*(other.m_edge_uids))[og_edge];
        (*(this->m_edge_uids))[copy_edge] = uid;
        this->m_uid_to_edge->insert({uid, copy_edge});
    }

    /*
     * Attributes maps for nodes and edges must be copied separately. This is because the
     * NodeArrays and EdgeArrays are bound to a specific ogdf::Graph instance and we want the maps
     * of this instance to point to the m_graph of this instance.
     */
    for (const auto &[key, original_array] : other.node_attrs())
    {
        this->m_node_attrs->try_emplace(key, *this->m_graph);

        auto &copy_array = this->m_node_attrs->at(key);
        for (const ogdf::node &og_node : other.graph().nodes)
        {
            const ogdf::node copy_node = og_to_copy_node.at(og_node);
            copy_array[copy_node] = original_array[og_node];
        }
    }

    for (const auto &[key, original_array] : other.edge_attrs())
    {
        this->m_edge_attrs->try_emplace(key, *this->m_graph);

        auto &copy_array = this->m_edge_attrs->at(key);
        for (const ogdf::edge &og_edge : other.graph().edges)
        {
            const ogdf::edge copy_edge = og_to_copy_edge.at(og_edge);
            copy_array[copy_edge] = original_array[og_edge];
        }
    }
}

graph_message::graph_message(const graphs::Graph &proto)
    : m_graph(std::make_unique<ogdf::Graph>())
    , m_uid{proto.uid()}
    , m_node_uids(std::make_unique<ogdf::NodeArray<uid_t>>(*(this->m_graph)))
    , m_edge_uids(std::make_unique<ogdf::EdgeArray<uid_t>>(*(this->m_graph)))
    , m_uid_to_node(std::make_unique<std::unordered_map<uid_t, ogdf::node>>())
    , m_uid_to_edge(std::make_unique<std::unordered_map<uid_t, ogdf::edge>>())
    , m_graph_attrs(std::make_unique<GraphAttributeMap>())
    , m_node_attrs(std::make_unique<NodeAttributeMap>())
    , m_edge_attrs(std::make_unique<EdgeAttributeMap>())
    , m_node_coords(std::make_unique<ogdf::NodeArray<coords_t>>(*(this->m_graph)))
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

    for (auto it = proto.attributes().begin(); it != proto.attributes().end(); ++it)
    {
        this->m_graph_attrs->emplace(std::make_pair(it->first, it->second));
    }

    /*
     * Parse attributes in separate pass since we can now directly allocate
     * arrays of the correct size.
     */
    for (auto node_it = proto.vertexlist().begin(); node_it != proto.vertexlist().end(); ++node_it)
    {
        const auto uid = node_it->uid();
        const double x = node_it->x();
        const double y = node_it->y();
        const double z = node_it->z();
        const auto attrs = node_it->attributes();

        const auto &node = (*(this->m_uid_to_node))[uid];
        const auto &node_attrs = node_it->attributes();

        (*(this->m_node_uids))[node] = uid;
        (*(this->m_node_coords))[node] = std::make_tuple(x, y, z);

        for (auto map_it = node_attrs.begin(); map_it != node_attrs.end(); ++map_it)
        {
            const auto &key = map_it->first;
            const auto &value = map_it->second;

            // Inserts NodeArray for this key if it does not exist yet
            this->m_node_attrs->try_emplace(key, *this->m_graph);
            this->m_node_attrs->at(key)[node] = value;
        }
    }

    for (auto edge_it = proto.edgelist().begin(); edge_it != proto.edgelist().end(); ++edge_it)
    {
        const auto uid = edge_it->uid();
        const auto &edge = (*(this->m_uid_to_edge))[uid];
        const auto &edge_attrs = edge_it->attributes();

        (*(this->m_edge_uids))[edge] = uid;

        for (auto map_it = edge_attrs.begin(); map_it != edge_attrs.end(); ++map_it)
        {
            const auto &key = map_it->first;
            const auto &value = map_it->second;

            // Inserts EdgeArray for this key if it does not exist yet
            this->m_edge_attrs->try_emplace(key, *this->m_graph);
            this->m_edge_attrs->at(key)[edge] = value;
        }
    }
}

graph_message::graph_message(const ogdf::Graph &graph, const ogdf::NodeArray<uid_t> &node_uids,
                             const ogdf::EdgeArray<uid_t> &edge_uids,
                             const ogdf::NodeArray<coords_t> &node_coords,
                             const GraphAttributeMap &graph_attrs,
                             const NodeAttributeMap &node_attrs, const EdgeAttributeMap &edge_attrs)
    : m_graph(std::make_unique<ogdf::Graph>())
    , m_uid{this->make_uid()}
    , m_node_uids(std::make_unique<ogdf::NodeArray<uid_t>>(*(this->m_graph)))
    , m_edge_uids(std::make_unique<ogdf::EdgeArray<uid_t>>(*(this->m_graph)))
    , m_uid_to_node(std::make_unique<std::unordered_map<uid_t, ogdf::node>>())
    , m_uid_to_edge(std::make_unique<std::unordered_map<uid_t, ogdf::edge>>())
    , m_graph_attrs(std::make_unique<GraphAttributeMap>(graph_attrs))
    , m_node_attrs(std::make_unique<NodeAttributeMap>())
    , m_edge_attrs(std::make_unique<EdgeAttributeMap>())
    , m_node_coords(std::make_unique<ogdf::NodeArray<coords_t>>(*(this->m_graph)))
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

    // ----------------- Node attributes
    for (const auto &[key, original_array] : node_attrs)
    {
        this->m_node_attrs->try_emplace(key, *this->m_graph);

        auto &copy_array = this->m_node_attrs->at(key);
        for (const ogdf::node &og_node : graph.nodes)
        {
            const ogdf::node copy_node = og_to_copy_node.at(og_node);
            copy_array[copy_node] = original_array[og_node];
        }
    }

    // ----------------- Edge attributes
    for (const auto &[key, original_array] : edge_attrs)
    {
        this->m_edge_attrs->try_emplace(key, *this->m_graph);

        auto &copy_array = this->m_edge_attrs->at(key);
        for (const ogdf::edge &og_edge : graph.edges)
        {
            const ogdf::edge copy_edge = og_to_copy_edge.at(og_edge);
            copy_array[copy_edge] = original_array[og_edge];
        }
    }

    // ----------------- Node coordinates
    for (const ogdf::node &og_node : graph.nodes)
    {
        const ogdf::node &copy_node = og_to_copy_node.at(og_node);
        (*(this->m_node_coords))[copy_node] = node_coords[og_node];
    }
}

graph_message::graph_message(std::unique_ptr<ogdf::Graph> graph,
                             std::unique_ptr<ogdf::NodeArray<uid_t>> node_uids,
                             std::unique_ptr<ogdf::EdgeArray<uid_t>> edge_uids,
                             std::unique_ptr<ogdf::NodeArray<coords_t>> node_coords,
                             std::unique_ptr<GraphAttributeMap> graph_attrs,
                             std::unique_ptr<NodeAttributeMap> node_attrs,
                             std::unique_ptr<EdgeAttributeMap> edge_attrs)
    : m_graph(std::move(graph))
    , m_uid{this->make_uid()}
    , m_node_uids(std::move(node_uids))
    , m_edge_uids(std::move(edge_uids))
    , m_uid_to_node(std::make_unique<std::unordered_map<uid_t, ogdf::node>>())
    , m_uid_to_edge(std::make_unique<std::unordered_map<uid_t, ogdf::edge>>())
    , m_graph_attrs(std::move(graph_attrs))
    , m_node_attrs(std::move(node_attrs))
    , m_edge_attrs(std::move(edge_attrs))
    , m_node_coords(std::move(node_coords))
{
    // Ensure we didn't get nullptrs
    if (!(this->m_graph && this->m_node_uids && this->m_edge_uids && this->m_node_coords &&
          this->m_graph_attrs && this->m_node_attrs && this->m_edge_attrs))
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

    // We can use the built-in copy constructor for this map only
    this->m_graph_attrs.reset(new GraphAttributeMap(other.graph_attrs()));

    // Make sure attribute maps are empty before copying contents
    this->m_node_attrs->clear();
    this->m_edge_attrs->clear();
    this->m_uid_to_node->clear();
    this->m_uid_to_edge->clear();

    this->m_uid = this->make_uid();
    this->m_node_uids = std::make_unique<ogdf::NodeArray<uid_t>>(*(this->m_graph));
    this->m_edge_uids = std::make_unique<ogdf::EdgeArray<uid_t>>(*(this->m_graph));

    this->m_node_coords = std::make_unique<ogdf::NodeArray<coords_t>>(*(this->m_graph));

    for (const ogdf::node &og_node : other.graph().nodes)
    {
        const ogdf::node &copy_node = og_to_copy_node.at(og_node);

        const uid_t uid = (*(other.m_node_uids))[og_node];
        (*(this->m_node_uids))[copy_node] = uid;
        this->m_uid_to_node->insert({uid, copy_node});

        (*(this->m_node_coords))[copy_node] = other.node_coords()[og_node];
    }

    for (const ogdf::edge &og_edge : other.graph().edges)
    {
        const ogdf::edge &copy_edge = og_to_copy_edge.at(og_edge);

        const uid_t uid = other.edge_uids()[og_edge];
        (*(this->m_edge_uids))[copy_edge] = uid;
        this->m_uid_to_edge->insert({uid, copy_edge});
    }

    /*
     * Attributes maps for nodes and edges must be copied separately. This is because the
     * NodeArrays and EdgeArrays are bound to a specific ogdf::Graph instance and we want the maps
     * of this instance to point to the m_graph of this instance.
     */
    for (const auto &[key, original_array] : other.node_attrs())
    {
        this->m_node_attrs->try_emplace(key, *this->m_graph);

        auto &copy_array = this->m_node_attrs->at(key);
        for (const ogdf::node &og_node : other.graph().nodes)
        {
            const ogdf::node copy_node = og_to_copy_node.at(og_node);
            copy_array[copy_node] = original_array[og_node];
        }
    }

    for (const auto &[key, original_array] : other.edge_attrs())
    {
        this->m_edge_attrs->try_emplace(key, *this->m_graph);

        auto &copy_array = this->m_edge_attrs->at(key);
        for (const ogdf::edge &og_edge : other.graph().edges)
        {
            const ogdf::edge copy_edge = og_to_copy_edge.at(og_edge);
            copy_array[copy_edge] = original_array[og_edge];
        }
    }

    return *this;
}

std::unique_ptr<graphs::Graph> graph_message::as_proto() const
{
    auto retval = std::make_unique<graphs::Graph>();

    retval->set_uid(this->m_uid);

    // Add general attributes
    if (this->m_graph_attrs)
    {
        auto *attr_map = retval->mutable_attributes();
        for (const auto &[key, attr] : *(this->m_graph_attrs))
        {
            (*attr_map)[key] = attr;
        }
    }

    // Add nodes
    const auto &node_uids = *(this->m_node_uids);
    for (const ogdf::node &node : this->m_graph->nodes)
    {
        graphs::Vertex *inserted = retval->add_vertexlist();
        inserted->set_uid(node_uids[node]);

        if (this->m_node_attrs)
        {
            for (const auto &[key, attr] : *(this->m_node_attrs))
            {
                (*inserted->mutable_attributes())[key] = attr[node];
            }
        }

        if (this->m_node_coords)
        {
            const auto &coords = (*(this->m_node_coords))[node];
            inserted->set_x(std::get<0>(coords));
            inserted->set_y(std::get<1>(coords));
            inserted->set_z(std::get<2>(coords));
        }
    }

    // Add edges
    const auto &edge_uids = *(this->m_edge_uids);
    for (const ogdf::edge &edge : this->m_graph->edges)
    {
        graphs::Edge *inserted = retval->add_edgelist();
        inserted->set_uid(edge_uids[edge]);

        inserted->set_invertexuid(node_uids[edge->source()]);
        inserted->set_outvertexuid(node_uids[edge->target()]);

        if (this->m_edge_attrs)
        {
            for (const auto &[key, attr] : *(this->m_edge_attrs))
            {
                (*inserted->mutable_attributes())[key] = attr[edge];
            }
        }
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

const GraphAttributeMap &graph_message::graph_attrs() const
{
    return *(this->m_graph_attrs);
}

const NodeAttributeMap &graph_message::node_attrs() const
{
    return *(this->m_node_attrs);
}

const EdgeAttributeMap &graph_message::edge_attrs() const
{
    return *(this->m_edge_attrs);
}

const ogdf::NodeArray<coords_t> &graph_message::node_coords() const
{
    return *(this->m_node_coords);
}

uid_t graph_message::make_uid() const
{
    using namespace std::chrono;

    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

}  // namespace server
