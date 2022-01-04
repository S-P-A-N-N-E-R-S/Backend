#include "networking/requests/generic_request.hpp"

#include "networking/exceptions.hpp"
#include "networking/utils.hpp"

namespace server {

generic_request::generic_request(const graphs::GenericRequest &proto_request)
    : abstract_request(request_type::GENERIC)
    , m_graph_message{proto_request.graph()}
    , m_node_coords{}
    , m_edge_costs{}
    , m_node_costs{}
    , m_graph_attributes{proto_request.graphattributes().begin(),
                         proto_request.graphattributes().end()}
    , m_static_attributes{proto_request.staticattributes().begin(),
                          proto_request.staticattributes().end()}
{
    // --- Parse node and edge costs as well as node coordinates if they were given.
    if (proto_request.vertexcoordinates_size() != 0 &&
        proto_request.vertexcoordinates_size() != this->m_graph_message.graph().numberOfNodes())
    {
        throw request_parse_error(
            "Vertex coordinates were provided but their number does not match the number of nodes",
            this->m_type);
    }
    else
    {
        this->m_node_coords = ogdf::NodeArray<node_coordinates>(this->m_graph_message.graph());

        utils::parse_node_attribute(proto_request.vertexcoordinates(), this->m_graph_message,
                                    this->m_node_coords);
    }

    if (proto_request.edgecosts_size() != 0 &&
        proto_request.edgecosts_size() != this->m_graph_message.graph().numberOfEdges())
    {
        throw request_parse_error(
            "Edge costs were provided but their number does not match the number of edges",
            this->m_type);
    }
    else
    {
        this->m_edge_costs = ogdf::EdgeArray<double>(this->m_graph_message.graph());

        utils::parse_edge_attribute(proto_request.edgecosts(), this->m_graph_message,
                                    this->m_edge_costs);
    }

    if (proto_request.vertexcosts_size() != 0 &&
        proto_request.vertexcosts_size() != this->m_graph_message.graph().numberOfNodes())
    {
        throw request_parse_error(
            "Node costs were provided but their number does not match the number of nodes",
            this->m_type);
    }
    else
    {
        this->m_node_costs = ogdf::NodeArray<double>(this->m_graph_message.graph());

        utils::parse_node_attribute(proto_request.vertexcosts(), this->m_graph_message,
                                    this->m_node_costs);
    }

    // --- Parse remaining node and edge attributes
    for (const auto &[name, attributes_msg] : proto_request.intattributes())
    {
        const auto type = attributes_msg.type();
        if (type == graphs::AttributeType::VERTEX)
        {
            auto [it, _] = this->m_node_int_attributes.emplace(
                name, ogdf::NodeArray<int64_t>(this->m_graph_message.graph()));

            utils::parse_node_attribute(attributes_msg.attributes(), this->m_graph_message,
                                        it->second);
        }
        else if (type == graphs::AttributeType::EDGE)
        {
            auto [it, _] = this->m_edge_int_attributes.emplace(
                name, ogdf::EdgeArray<int64_t>(this->m_graph_message.graph()));

            utils::parse_edge_attribute(attributes_msg.attributes(), this->m_graph_message,
                                        it->second);
        }
    }

    for (const auto &[name, attributes_msg] : proto_request.doubleattributes())
    {
        const auto type = attributes_msg.type();
        if (type == graphs::AttributeType::VERTEX)
        {
            auto [it, _] = this->m_node_double_attributes.emplace(
                name, ogdf::NodeArray<double>(this->m_graph_message.graph()));

            utils::parse_node_attribute(attributes_msg.attributes(), this->m_graph_message,
                                        it->second);
        }
        else if (type == graphs::AttributeType::EDGE)
        {
            auto [it, _] = this->m_edge_double_attributes.emplace(
                name, ogdf::EdgeArray<double>(this->m_graph_message.graph()));

            utils::parse_edge_attribute(attributes_msg.attributes(), this->m_graph_message,
                                        it->second);
        }
    }
}

const graph_message *generic_request::graph_message() const
{
    return &(this->m_graph_message);
}

server::graph_message generic_request::take_graph_message()
{
    return std::move(this->m_graph_message);
}

const ogdf::NodeArray<node_coordinates> *generic_request::node_coords() const
{
    return &(this->m_node_coords);
}

ogdf::NodeArray<node_coordinates> generic_request::take_node_coords()
{
    return std::move(this->m_node_coords);
}

const ogdf::EdgeArray<double> *generic_request::edge_costs() const
{
    return &(this->m_edge_costs);
}

ogdf::EdgeArray<double> generic_request::take_edge_costs()
{
    return std::move(this->m_edge_costs);
}

const ogdf::NodeArray<double> *generic_request::node_costs() const
{
    return &(this->m_node_costs);
}

ogdf::NodeArray<double> generic_request::take_node_costs()
{
    return std::move(this->m_node_costs);
}

const ogdf::NodeArray<int64_t> *generic_request::node_int_attribute(const std::string &name) const
{
    auto it = this->m_node_int_attributes.find(name);
    if (it != this->m_node_int_attributes.end())
    {
        return &(it->second);
    }

    return nullptr;
}

ogdf::NodeArray<int64_t> generic_request::take_node_int_attribute(const std::string &name)
{
    auto it = this->m_node_int_attributes.find(name);
    if (it != this->m_node_int_attributes.end())
    {
        return std::move(it->second);
    }

    return {};
}

const ogdf::NodeArray<double> *generic_request::node_double_attribute(const std::string &name) const
{
    auto it = this->m_node_double_attributes.find(name);
    if (it != this->m_node_double_attributes.end())
    {
        return &(it->second);
    }

    return nullptr;
}

ogdf::NodeArray<double> generic_request::take_node_double_attribute(const std::string &name)
{
    auto it = this->m_node_double_attributes.find(name);
    if (it != this->m_node_double_attributes.end())
    {
        return std::move(it->second);
    }

    return {};
}

const ogdf::EdgeArray<int64_t> *generic_request::edge_int_attribute(const std::string &name) const
{
    auto it = this->m_edge_int_attributes.find(name);
    if (it != this->m_edge_int_attributes.end())
    {
        return &(it->second);
    }

    return nullptr;
}

ogdf::EdgeArray<int64_t> generic_request::take_edge_int_attribute(const std::string &name)
{
    auto it = this->m_edge_int_attributes.find(name);
    if (it != this->m_edge_int_attributes.end())
    {
        return std::move(it->second);
    }

    return {};
}

const ogdf::EdgeArray<double> *generic_request::edge_double_attribute(const std::string &name) const
{
    auto it = this->m_edge_double_attributes.find(name);
    if (it != this->m_edge_double_attributes.end())
    {
        return &(it->second);
    }

    return nullptr;
}

ogdf::EdgeArray<double> generic_request::take_edge_double_attribute(const std::string &name)
{
    auto it = this->m_edge_double_attributes.find(name);
    if (it != this->m_edge_double_attributes.end())
    {
        return std::move(it->second);
    }

    return {};
}

const std::unordered_map<std::string, std::string> &generic_request::graph_attributes() const
{
    return this->m_graph_attributes;
}

const std::unordered_map<std::string, std::string> &generic_request::static_attributes() const
{
    return this->m_static_attributes;
}

}  // namespace server
