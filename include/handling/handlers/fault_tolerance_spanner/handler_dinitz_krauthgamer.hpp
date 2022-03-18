#pragma once

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/graphalg/SpannerBasicGreedy.h>
#include <ogdf/graphalg/SpannerDinitzKrauthgamer.h>

#include "handling/handlers/abstract_handler.hpp"
#include "networking/requests/generic_request.hpp"

namespace server {

class dinitz_krauthgamer_handler : public abstract_handler
{
public:
    dinitz_krauthgamer_handler(std::unique_ptr<abstract_request> request);

    virtual ~dinitz_krauthgamer_handler() = default;

    virtual handle_return handle() override;

    static graphs::HandlerInformation handler_information();

    static std::string name();

private:
    std::unique_ptr<generic_request> m_request;
};

}  // namespace server
