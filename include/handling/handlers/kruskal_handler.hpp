#pragma once

#include "handling/handlers/abstract_handler.hpp"
#include "networking/requests/generic_request.hpp"

namespace server {

/**
 * @brief Handler for building MSTs with Kruskal algorithm. Requests need as input a graph,
 * edge weights, node coordinates and a source node. The response sends a graph, edge weights 
 * and node coordinates.
 * 
 */
class kruskal_handler : public abstract_handler
{
public:
    static std::string name();

    static graphs::HandlerInformation handler_information();

    kruskal_handler(std::unique_ptr<abstract_request> request);

    virtual handle_return handle() override;

private:
    std::unique_ptr<generic_request> m_request;
};

}  // namespace server
