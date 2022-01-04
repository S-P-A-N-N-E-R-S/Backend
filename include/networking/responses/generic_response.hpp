#pragma once

#include "networking/messages/graph_message.hpp"
#include "networking/messages/node_coordinates.hpp"
#include "networking/responses/abstract_response.hpp"

#include "generic_container.pb.h"

namespace server {

class generic_response : public abstract_response
{
public:
    template <typename T>
    using attribute_map = std::unordered_map<std::string, T>;

    generic_response(const graph_message *const graph,
                     const ogdf::NodeArray<node_coordinates> *const node_coords,
                     const ogdf::EdgeArray<double> *const edge_costs,
                     const ogdf::NodeArray<double> *const vertex_costs,
                     const attribute_map<ogdf::NodeArray<int64_t>> *const node_int_attributes,
                     const attribute_map<ogdf::NodeArray<double>> *const node_double_attributes,
                     const attribute_map<ogdf::EdgeArray<int64_t>> *const edge_int_attributes,
                     const attribute_map<ogdf::EdgeArray<double>> *const edge_double_attributes,
                     const attribute_map<std::string> *const graph_attributes, status_code status);
    virtual ~generic_response() = default;

    graphs::GenericResponse as_proto();

private:
    graphs::Graph m_proto_graph;

    google::protobuf::RepeatedPtrField<graphs::VertexCoordinates> m_vertex_coords;
    google::protobuf::RepeatedField<double> m_edge_costs;
    google::protobuf::RepeatedField<double> m_vertex_costs;

    google::protobuf::Map<std::string, graphs::IntAttributes> m_int_attributes;
    google::protobuf::Map<std::string, graphs::DoubleAttributes> m_double_attributes;
    google::protobuf::Map<std::string, std::string> m_graph_attributes;

    /*
     * The Protobuf representation `GenericResponse` also contains a `staticAttributes` field.
     * Since this field is not to meant to be set by the users but rather to be copied straight
     * from the corresponding request.
     */
};

}  // namespace server
