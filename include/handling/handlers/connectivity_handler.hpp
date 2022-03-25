#pragma once

#include <ogdf/graphalg/ConnectivityTester.h>
#include <chrono>
#include "handling/handlers/abstract_handler.hpp"
#include "networking/exceptions.hpp"
#include "networking/requests/generic_request.hpp"
#include "networking/responses/generic_response.hpp"
#include "networking/responses/response_factory.hpp"
#include "util/GraphProperties.h"

namespace server {

class connectivity_handler : public abstract_handler
{
public:
    static std::string name();

    static graphs::HandlerInformation handler_information();

    connectivity_handler(std::unique_ptr<abstract_request> request);

    virtual handle_return handle() override;

private:
    std::unique_ptr<generic_request> m_request;
};

}  // namespace server
