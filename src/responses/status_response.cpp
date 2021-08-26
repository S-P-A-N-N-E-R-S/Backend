#include <networking/responses/status_response.hpp>

namespace server {
status_response::status_response(graphs::StatusResponse &&statusResponse, status_code status)
    : abstract_response(response_type::STATUS, status)
    , m_statusResponse(std::move(statusResponse))
{
}

graphs::StatusResponse status_response::take_status_response()
{
    return std::move(m_statusResponse);
}

}  // namespace server
