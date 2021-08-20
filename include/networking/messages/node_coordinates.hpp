#pragma once

#include "handlers/shortest_path.pb.h"

namespace server {

struct node_coordinates {
    // Needed so we can initialize a ogdf::NodeArray<node_coordinates>
    node_coordinates() = default;

    node_coordinates(double x, double y, double z);
    node_coordinates(const graphs::VertexCoordinates &proto_coords);

    graphs::VertexCoordinates as_proto() const;

    double m_x{};
    double m_y{};
    double m_z{};
};

}  // namespace server
