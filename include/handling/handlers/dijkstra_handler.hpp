#pragma once

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/graphalg/Dijkstra.h>

#include "handling/handler_factory.hpp"
#include "handling/handlers/abstract_handler.hpp"
#include "networking/messages/node_coordinates.hpp"
#include "networking/requests/shortest_path_request.hpp"
#include "networking/responses/shortest_path_response.hpp"

namespace server {

class DijkstraHandler : public abstract_handler
{
public:
    DijkstraHandler(std::unique_ptr<abstract_request> request);

    virtual std::pair<graphs::ResponseContainer, long> handle() override;

    static graphs::HandlerInformation handler_information();

private:
    std::unique_ptr<abstract_request> m_request;
    const graph_message *m_req_message;
    bool m_directed;
    ogdf::node m_start;
    const ogdf::Graph *m_graph;
    const ogdf::EdgeArray<double> *m_weights;
    const ogdf::NodeArray<node_coordinates> *m_node_coords;
};

}  // namespace server
