#pragma once

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/graphalg/FaultTolerantSpannerBodwinDinitz.h>

#include "handling/handlers/abstract_handler.hpp"
#include "networking/requests/generic_request.hpp"

namespace server {

class bodwin_dinitz_handler : public abstract_handler
{
public:
    bodwin_dinitz_handler(std::unique_ptr<abstract_request> request);

    virtual ~bodwin_dinitz_handler() = default;

    virtual handle_return handle() override;

    static graphs::HandlerInformation handler_information();

    static std::string name();

private:
    std::unique_ptr<generic_request> m_request;
};

}   // namespace server
