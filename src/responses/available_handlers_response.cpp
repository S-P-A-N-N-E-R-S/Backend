#include <networking/responses/available_handlers_response.hpp>

namespace server {

available_handlers_response::available_handlers_response(
    std::unique_ptr<graphs::AvailableHandlersResponse> available_handlers, status_code status)
    : abstract_response(response_type::AVAILABLE_HANDLERS, status)
    , m_available_handlers(std::move(available_handlers))
{
}

const graphs::AvailableHandlersResponse *available_handlers_response::available_handlers() const
{
    return this->m_available_handlers.get();
}

std::unique_ptr<graphs::AvailableHandlersResponse>
    available_handlers_response::take_available_handlers()
{
    return std::move(this->m_available_handlers);
}

}  // namespace server
