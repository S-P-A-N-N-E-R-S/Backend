#include "networking/responses/generic_response.hpp"

#include "networking/utils.hpp"

namespace server {

generic_response::generic_response(
    const graph_message *const graph, const ogdf::NodeArray<node_coordinates> *const node_coords,
    const ogdf::EdgeArray<double> *const edge_costs,
    const ogdf::NodeArray<double> *const vertex_costs,
    const attribute_map<ogdf::NodeArray<int64_t>> *const node_int_attributes,
    const attribute_map<ogdf::NodeArray<double>> *const node_double_attributes,
    const attribute_map<ogdf::EdgeArray<int64_t>> *const edge_int_attributes,
    const attribute_map<ogdf::EdgeArray<double>> *const edge_double_attributes,
    const attribute_map<std::string> *const graph_attributes, status_code status)
    : abstract_response{response_type::GENERIC, status}
{
    if (graph)
    {
        this->m_proto_graph = graphs::Graph{graph->as_proto()};
    }

    if (node_coords)
    {
        this->m_vertex_coords = google::protobuf::RepeatedPtrField<graphs::VertexCoordinates>{};

        utils::serialize_node_attribute(
            *node_coords, *graph, this->m_vertex_coords,
            std::function<graphs::VertexCoordinates(const node_coordinates &)>(
                [](const auto &coords) {
                    return coords.as_proto();
                }));
    }

    if (edge_costs)
    {
        this->m_edge_costs = google::protobuf::RepeatedField<double>{};

        utils::serialize_edge_attribute(*edge_costs, *graph, this->m_edge_costs);
    }

    if (vertex_costs)
    {
        this->m_vertex_costs = google::protobuf::RepeatedField<double>{};

        utils::serialize_node_attribute(*vertex_costs, *graph, this->m_vertex_costs);
    }

    if (node_int_attributes)
    {
        this->m_int_attributes = google::protobuf::Map<std::string, graphs::IntAttributes>{};

        for (const auto &[name, attributes] : *node_int_attributes)
        {
            this->m_int_attributes[name] = graphs::IntAttributes{};
            this->m_int_attributes[name].set_type(graphs::AttributeType::VERTEX);

            auto &proto_attributes = *(this->m_int_attributes[name].mutable_attributes());
            utils::serialize_node_attribute(attributes, *graph, proto_attributes);
        }
    }

    if (node_double_attributes)
    {
        this->m_double_attributes = google::protobuf::Map<std::string, graphs::DoubleAttributes>{};

        for (const auto &[name, attributes] : *node_double_attributes)
        {
            this->m_double_attributes[name] = graphs::DoubleAttributes{};
            this->m_double_attributes[name].set_type(graphs::AttributeType::VERTEX);

            auto &proto_attributes = *(this->m_double_attributes[name].mutable_attributes());
            utils::serialize_node_attribute(attributes, *graph, proto_attributes);
        }
    }

    if (edge_int_attributes)
    {
        for (const auto &[name, attributes] : *edge_int_attributes)
        {
            this->m_int_attributes[name] = graphs::IntAttributes{};
            this->m_int_attributes[name].set_type(graphs::AttributeType::EDGE);

            auto &proto_attributes = *(this->m_int_attributes[name].mutable_attributes());
            utils::serialize_edge_attribute(attributes, *graph, proto_attributes);
        }
    }

    if (edge_double_attributes)
    {
        for (const auto &[name, attributes] : *edge_double_attributes)
        {
            this->m_double_attributes[name] = graphs::DoubleAttributes{};
            this->m_double_attributes[name].set_type(graphs::AttributeType::EDGE);

            auto &proto_attributes = *(this->m_double_attributes[name].mutable_attributes());
            utils::serialize_edge_attribute(attributes, *graph, proto_attributes);
        }
    }

    if (graph_attributes)
    {
        this->m_graph_attributes = google::protobuf::Map<std::string, std::string>(
            graph_attributes->begin(), graph_attributes->end());
    }
}

graphs::GenericResponse generic_response::as_proto()
{
    // TODO: Should be optimized once we use arena allocation
    graphs::GenericResponse proto_response;

    *(proto_response.mutable_graph()) = std::move(this->m_proto_graph);
    *(proto_response.mutable_vertexcoordinates()) = std::move(this->m_vertex_coords);
    *(proto_response.mutable_edgecosts()) = std::move(this->m_edge_costs);
    *(proto_response.mutable_vertexcosts()) = std::move(this->m_vertex_costs);
    *(proto_response.mutable_intattributes()) = std::move(this->m_int_attributes);
    *(proto_response.mutable_doubleattributes()) = std::move(this->m_double_attributes);
    *(proto_response.mutable_graphattributes()) = std::move(this->m_graph_attributes);

    return proto_response;
}

}  // namespace server
