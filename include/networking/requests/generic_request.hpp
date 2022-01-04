#pragma once

#include "networking/messages/graph_message.hpp"
#include "networking/messages/node_coordinates.hpp"
#include "networking/requests/abstract_request.hpp"

#include "generic_container.pb.h"

namespace server {

class generic_request : public abstract_request
{
public:
    generic_request(const graphs::GenericRequest &proto_request);
    virtual ~generic_request() = default;

    const server::graph_message *graph_message() const;
    server::graph_message take_graph_message();

    const ogdf::NodeArray<node_coordinates> *node_coords() const;
    ogdf::NodeArray<node_coordinates> take_node_coords();

    const ogdf::EdgeArray<double> *edge_costs() const;
    ogdf::EdgeArray<double> take_edge_costs();

    const ogdf::NodeArray<double> *node_costs() const;
    ogdf::NodeArray<double> take_node_costs();

    const ogdf::NodeArray<int64_t> *node_int_attribute(const std::string &name) const;
    ogdf::NodeArray<int64_t> take_node_int_attribute(const std::string &name);

    const ogdf::NodeArray<double> *node_double_attribute(const std::string &name) const;
    ogdf::NodeArray<double> take_node_double_attribute(const std::string &name);

    const ogdf::EdgeArray<int64_t> *edge_int_attribute(const std::string &name) const;
    ogdf::EdgeArray<int64_t> take_edge_int_attribute(const std::string &name);

    const ogdf::EdgeArray<double> *edge_double_attribute(const std::string &name) const;
    ogdf::EdgeArray<double> take_edge_double_attribute(const std::string &name);

    const std::unordered_map<std::string, std::string> &graph_attributes() const;

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
