#pragma once

#include "networking/messages/graph_message.hpp"
#include "networking/messages/node_coordinates.hpp"
#include "networking/requests/abstract_request.hpp"

#include "generic_container.pb.h"

namespace server {

class generic_request : public abstract_request
{
public:
    /**
     * @brief Constructs from the protocol buffer message.
     */
    generic_request(const graphs::GenericRequest &proto_request);
    virtual ~generic_request() = default;

    /**
     * @brief Immutable access to the `graph_message` of this request.
     *
     * @return `graph_message` of this request
     */
    const server::graph_message *graph_message() const;

    /**
     * @brief Mutable access to the `graph_message` of this request. Invalidates this object.
     *
     * @return `graph_message` of this request
     */
    server::graph_message take_graph_message();

    /**
     * @brief Immutable access to the node coordinates of this request.
     *
     * @return `node_coordinates` of the graph of this request
     */
    const ogdf::NodeArray<node_coordinates> *node_coords() const;

    /**
     * @brief Mutable access to the node coordinates of this request. Invalidates this object.
     *
     * @return `node_coordinates` of the graph of this request
     */
    ogdf::NodeArray<node_coordinates> take_node_coords();

    /**
     * @brief Immutable access to the edge costs of this request.
     *
     * @return Edge costs of the graph of this request
     */
    const ogdf::EdgeArray<double> *edge_costs() const;

    /**
     * @brief Mutable access to the edge costs of this request. Invalidates this object.
     *
     * @return Edge costs of the graph of this request
     */
    ogdf::EdgeArray<double> take_edge_costs();

    /**
     * @brief Immutable access to the node costs of this request.
     *
     * @return Node costs of the graph of this request
     */
    const ogdf::NodeArray<double> *node_costs() const;

    /**
     * @brief Mutable access to the node costs of this request. Invalidates this object.
     *
     * @return Node costs of the graph of this request
     */
    ogdf::NodeArray<double> take_node_costs();

    /**
     * @brief Immutable access to the node integer attribute with a specific name of this request.
     *
     * @param name Name of the requested attribute
     *
     * @return Node integer attribute with the name `name`
     */
    const ogdf::NodeArray<int64_t> *node_int_attribute(const std::string &name) const;

    /**
     * @brief Mutable access to the node integer attribute with a specific name of this request.
     *        Invalidates this object.
     *
     * @param name Name of the requested attribute
     *
     * @return Node integer attribute with the name `name`
     */
    ogdf::NodeArray<int64_t> take_node_int_attribute(const std::string &name);

    /**
     * @brief Immutable access to the node double attribute with a specific name of this request.
     *
     * @param name Name of the requested attribute
     *
     * @return Node double attribute with the name `name`
     */
    const ogdf::NodeArray<double> *node_double_attribute(const std::string &name) const;

    /**
     * @brief Mutable access to the node double attribute with a specific name of this request.
     *        Invalidates this object.
     *
     * @param name Name of the requested attribute
     *
     * @return Node double attribute with the name `name`
     */
    ogdf::NodeArray<double> take_node_double_attribute(const std::string &name);

    /**
     * @brief Immutable access to the edge integer attribute with a specific name of this request.
     *
     * @param name Name of the requested attribute
     *
     * @return Edge integer attribute with the name `name`
     */
    const ogdf::EdgeArray<int64_t> *edge_int_attribute(const std::string &name) const;

    /**
     * @brief Mutable access to the edge integer attribute with a specific name of this request.
     *        Invalidates this object.
     *
     * @param name Name of the requested attribute
     *
     * @return Edge integer attribute with the name `name`
     */
    ogdf::EdgeArray<int64_t> take_edge_int_attribute(const std::string &name);

    /**
     * @brief Immutable access to the edge double attribute with a specific name of this request.
     *
     * @param name Name of the requested attribute
     *
     * @return Edge double attribute with the name `name`
     */
    const ogdf::EdgeArray<double> *edge_double_attribute(const std::string &name) const;

    /**
     * @brief Mutable access to the edge double attribute with a specific name of this request.
     *        Invalidates this object.
     *
     * @param name Name of the requested attribute
     *
     * @return Edge double attribute with the name `name`
     */
    ogdf::EdgeArray<double> take_edge_double_attribute(const std::string &name);

    /**
     * @brief Immutable access to the general graph attributes of this request.
     *
     * @return Map of graph attributes' names to their values
     */
    const std::unordered_map<std::string, std::string> &graph_attributes() const;

    /**
     * @brief Immutable access to the static attributes of this request.
     *
     * @return Map of static attributes' names to their values
     */
    const std::unordered_map<std::string, std::string> &static_attributes() const;

private:
    template <typename T>
    using AttributeMap = std::unordered_map<std::string, T>;

    server::graph_message m_graph_message;

    ogdf::NodeArray<node_coordinates> m_node_coords;
    ogdf::EdgeArray<double> m_edge_costs;
    ogdf::NodeArray<double> m_node_costs;

    AttributeMap<ogdf::NodeArray<int64_t>> m_node_int_attributes;
    AttributeMap<ogdf::NodeArray<double>> m_node_double_attributes;

    AttributeMap<ogdf::EdgeArray<int64_t>> m_edge_int_attributes;
    AttributeMap<ogdf::EdgeArray<double>> m_edge_double_attributes;

    std::unordered_map<std::string, std::string> m_graph_attributes;

    std::unordered_map<std::string, std::string> m_static_attributes;
};

}  // namespace server
