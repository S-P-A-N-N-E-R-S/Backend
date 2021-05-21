#include "networking/requests/abstract_request.hpp"

namespace server {

request_type abstract_request::type() const
{
    return this->m_type;
}

abstract_request::abstract_request(request_type type)
    : m_type{type}
{
}

}  // namespace server
