#pragma once

#include "networking/responses/response_type.hpp"

#include <exception>

namespace server {

class request_parse_error : public std::exception
{
public:
    request_parse_error(const char *msg, request_type type, std::string_view handler_type = "")
        : std::exception()
        , m_message{msg}
        , m_request_type{type}
        , m_handler_type{handler_type}
    {
    }

    const char *what() const noexcept
    {
        return this->m_message;
    }

    server::request_type request_type() const noexcept
    {
        return this->m_request_type;
    }

    const std::string_view &handler_type() const noexcept
    {
        return this->m_handler_type;
    }

private:
    const char *m_message;
    server::request_type m_request_type;
    std::string_view m_handler_type;
};

}  // namespace server
