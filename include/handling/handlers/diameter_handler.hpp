#pragma once

#include <ogdf/graphalg/GraphProperties.h>
#include <chrono>
#include "handling/handlers/abstract_handler.hpp"
#include "networking/exceptions.hpp"
#include "networking/requests/generic_request.hpp"
#include "networking/responses/generic_response.hpp"
#include "networking/responses/response_factory.hpp"

namespace server {

class diameter_handler : public abstract_handler
{
public:
    static std::string name();

    static graphs::HandlerInformation handler_information();

    diameter_handler(std::unique_ptr<abstract_request> request);

    virtual handle_return handle() override;

private:
    std::unique_ptr<generic_request> m_request;
};

}  // namespace server
