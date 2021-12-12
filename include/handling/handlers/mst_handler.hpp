#pragma once

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/extended_graph_alg.h>

#include "handling/handlers/abstract_handler.hpp"
#include "networking/requests/generic_request.hpp"

namespace server {

class mst_handler : public abstract_handler
{
public:
    mst_handler(std::unique_ptr<abstract_request> request);

    virtual ~mst_handler() = default;

    virtual std::pair<graphs::ResponseContainer, long> handle() override;

    static graphs::HandlerInformation handler_information();

    static std::string name();

private:
    std::unique_ptr<generic_request> m_request;
};

}  // namespace server
