#pragma once

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/graphalg/Dijkstra.h>

#include "handling/handlers/abstract_handler.hpp"
#include "networking/requests/generic_request.hpp"

namespace server {

/**
 * @brief Handler for SSSP queries with Dijkstra's algorithm. Requests need as input a graph, 
 * edge weights, node coordinates and a source node. The response sends a directed graph,
 * where each edge points at the predecessor node on the shortest path, edge weights and 
 * node coordinates
 * 
 */
class dijkstra_handler : public abstract_handler
{
public:
    dijkstra_handler(std::unique_ptr<abstract_request> request);

    virtual ~dijkstra_handler() = default;

    virtual handle_return handle() override;

    static graphs::HandlerInformation handler_information();

    static std::string name();

private:
    std::unique_ptr<generic_request> m_request;
};

}  // namespace server
