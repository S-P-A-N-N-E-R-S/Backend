#pragma once

#include "networking/responses/response_type.hpp"

#include <exception>

namespace server {

class request_parse_error : public std::exception
{
public:
    request_parse_error(const char *msg, request_type type)
        : std::exception()
        , m_message{msg}
        , m_request_type{type}
    {
    }

    const char *what() const noexcept
    {
        return this->m_message;
    }

    request_type type() const noexcept
    {
        return this->m_request_type;
    }

private:
    const char *m_message;
    request_type m_request_type;
};

}  // namespace server
