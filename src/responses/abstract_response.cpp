#include "networking/responses/abstract_response.hpp"

namespace server {

response_type abstract_response::type() const
{
    return this->m_type;
}

status_code abstract_response::status() const
{
    return this->m_status;
}

abstract_response::abstract_response(response_type type, status_code status)
    : m_type{type}
    , m_status{status}
{
}

}  // namespace server
