#pragma once

#include "networking/requests/request_type.hpp"
#include "networking/responses/response_type.hpp"

#include <pqxx/result.hxx>

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

class row_access_error : public std::exception
{
public:
    row_access_error(const char *msg, const pqxx::result &result)
        : std::exception()
        , m_message{msg}
        , m_query{result.query()}
    {
    }

    // Use this constructor if you don't have a pqxx::result at hand
    row_access_error(const char *msg)
        : std::exception()
        , m_message{msg}
        , m_query{}
    {
    }

    const char *what() const noexcept
    {
        return this->m_message;
    }

    const std::string &query() const noexcept
    {
        return this->m_query;
    }

private:
    const char *const m_message;
    const std::string m_query;
};

class response_error : public std::exception
{
public:
    response_error(const char *msg, response_type type, std::string_view handler_type = "")
        : std::exception()
        , m_message{msg}
        , m_response_type{type}
        , m_handler_type{handler_type}
    {
    }

    const char *what() const noexcept
    {
        return this->m_message;
    }

    server::response_type response_type() const noexcept
    {
        return this->m_response_type;
    }

    const std::string_view &handler_type() const noexcept
    {
        return this->m_handler_type;
    }

private:
    const char *m_message;
    server::response_type m_response_type;
    std::string_view m_handler_type;
};

}  // namespace server
