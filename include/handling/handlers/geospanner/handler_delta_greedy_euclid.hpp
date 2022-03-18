#pragma once

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/graphalg/geospanner/GeospannerPredefined.h>

#include "handling/handlers/abstract_handler.hpp"
#include "networking/requests/generic_request.hpp"

namespace server {

/**
 * @brief Handler for the path greedy geospanner algorithm. Requests need as input a graph,
 * node coordinates, a stretch factor t and a delta value. The response sends an undirected graph (which is 
 * a t-spanner of the original graph), edge weights and node coordinates
 * 
 */
class delta_greedy_euclid_handler : public abstract_handler
{
public:
    delta_greedy_euclid_handler(std::unique_ptr<abstract_request> request);

    virtual ~delta_greedy_euclid_handler() = default;

    virtual handle_return handle() override;

    static graphs::HandlerInformation handler_information();

    static std::string name();

private:
    std::unique_ptr<generic_request> m_request;
};

}  // namespace server