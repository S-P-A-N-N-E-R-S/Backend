#pragma once

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/graphalg/Dijkstra.h>

#include "handling/handlers/abstract_handler.hpp"
#include "networking/requests/generic_request.hpp"

namespace server {

class dijkstra_handler : public abstract_handler
{
public:
    dijkstra_handler(std::unique_ptr<abstract_request> request);

    virtual std::pair<graphs::ResponseContainer, long> handle() override;

    static graphs::HandlerInformation handler_information();

private:
    std::unique_ptr<generic_request> m_request;
};

}  // namespace server
