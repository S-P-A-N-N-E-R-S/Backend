#pragma once

#include <memory>

#include "networking/messages/graph_message.hpp"
#include "networking/messages/node_coordinates.hpp"
#include "networking/responses/abstract_response.hpp"

#include "available_handlers.pb.h"

namespace server {

class available_handlers_response : public abstract_response
{
public:
    available_handlers_response(
        std::unique_ptr<graphs::AvailableHandlersResponse> available_handlers, status_code status);
    virtual ~available_handlers_response() = default;

    const graphs::AvailableHandlersResponse *available_handlers() const;
    std::unique_ptr<graphs::AvailableHandlersResponse> take_available_handlers();

private:
    std::unique_ptr<graphs::AvailableHandlersResponse> m_available_handlers;
};

}  // namespace server
