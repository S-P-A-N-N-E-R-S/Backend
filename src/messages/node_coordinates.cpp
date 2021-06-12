#include "networking/messages/node_coordinates.hpp"

namespace server {

node_coordinates::node_coordinates(double x, double y, double z)
    : m_x{x}
    , m_y{y}
    , m_z{z}
{
}

node_coordinates::node_coordinates(const graphs::VertexCoordinates &proto_coords)
    : m_x{proto_coords.x()}
    , m_y{proto_coords.y()}
    , m_z{proto_coords.z()}
{
}

graphs::VertexCoordinates node_coordinates::as_proto() const
{
    graphs::VertexCoordinates proto;

    proto.set_x(this->m_x);
    proto.set_y(this->m_y);
    proto.set_z(this->m_z);

    return proto;
}

}  // namespace server
