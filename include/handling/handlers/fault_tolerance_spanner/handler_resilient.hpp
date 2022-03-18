#pragma once

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/graphalg/ResilientSpanner.h>
#include <ogdf/graphalg/SpannerBasicGreedy.h>

#include "handling/handlers/abstract_handler.hpp"
#include "networking/requests/generic_request.hpp"

namespace server {

class resilient_handler : public abstract_handler
{
public:
    resilient_handler(std::unique_ptr<abstract_request> request);

    virtual ~resilient_handler() = default;

    virtual handle_return handle() override;

    static graphs::HandlerInformation handler_information();

    static std::string name();

private:
    std::unique_ptr<generic_request> m_request;
};

}   // namespace server
