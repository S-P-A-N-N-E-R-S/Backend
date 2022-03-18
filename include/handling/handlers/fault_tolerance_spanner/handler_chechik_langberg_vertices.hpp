#pragma once

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/graphalg/SpannerChechikLangbergVertexSimple.h>

#include "handling/handlers/abstract_handler.hpp"
#include "networking/requests/generic_request.hpp"

namespace server {

class chechik_langberg_vertices_handler : public abstract_handler
{
public:
    chechik_langberg_vertices_handler(std::unique_ptr<abstract_request> request);

    virtual ~chechik_langberg_vertices_handler() = default;

    virtual handle_return handle() override;

    static graphs::HandlerInformation handler_information();

    static std::string name();

private:
    std::unique_ptr<generic_request> m_request;
};

}   // namespace server
