#pragma once

#include <memory>

#include "networking/messages/graph_message.hpp"
#include "networking/messages/node_coordinates.hpp"
#include "networking/requests/abstract_request.hpp"

#include "handlers/shortest_path.pb.h"

namespace server {

class shortest_path_request : public abstract_request
{
public:
    shortest_path_request(const graphs::ShortestPathRequest &proto_request);
    virtual ~shortest_path_request() = default;

    const server::graph_message *graph_message() const;
    std::unique_ptr<server::graph_message> take_graph_message();

    uid_t start_uid() const;
    uid_t end_uid() const;

    const ogdf::EdgeArray<double> *edge_costs() const;
    std::unique_ptr<ogdf::EdgeArray<double>> take_edge_costs();

    const ogdf::NodeArray<node_coordinates> *node_coords() const;
    std::unique_ptr<ogdf::NodeArray<node_coordinates>> take_node_coords();

private:
    std::unique_ptr<server::graph_message> m_graph_message;

    uid_t m_start_uid;
    uid_t m_end_uid;

    std::unique_ptr<ogdf::EdgeArray<double>> m_edge_costs;
    std::unique_ptr<ogdf::NodeArray<node_coordinates>> m_node_coords;
};

}  // namespace server
