#pragma once

#include <memory>

#include "networking/messages/graph_message.hpp"
#include "networking/messages/node_coordinates.hpp"
#include "networking/responses/abstract_response.hpp"

namespace server {

// The getters without the `take_` prefix return pointers because they will be nullptr after they
// have been returned by a getter with the `take_` prefix.
class shortest_path_response : public abstract_response
{
public:
    shortest_path_response(std::unique_ptr<server::graph_message> shortest_path,
                           std::unique_ptr<ogdf::EdgeArray<double>> edge_costs,
                           std::unique_ptr<ogdf::NodeArray<node_coordinates>> node_coords,
                           status_code status);
    virtual ~shortest_path_response() = default;

    const graphs::Graph *proto_graph() const;
    std::unique_ptr<graphs::Graph> take_proto_graph();

    const google::protobuf::Map<uid_t, double> *edge_costs() const;
    std::unique_ptr<google::protobuf::Map<uid_t, double>> take_edge_costs();

    const google::protobuf::Map<uid_t, graphs::VertexCoordinates> *vertex_coords() const;
    std::unique_ptr<google::protobuf::Map<uid_t, graphs::VertexCoordinates>> take_vertex_coords();

private:
    std::unique_ptr<graphs::Graph> m_proto_graph;
    std::unique_ptr<google::protobuf::Map<uid_t, double>> m_edge_costs;
    std::unique_ptr<google::protobuf::Map<uid_t, graphs::VertexCoordinates>> m_vertex_coords;
};

}  // namespace server
