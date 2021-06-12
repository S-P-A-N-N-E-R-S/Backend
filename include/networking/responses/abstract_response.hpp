#pragma once

#include "networking/responses/response_type.hpp"
#include "networking/responses/status_code.hpp"

namespace server {

class abstract_response
{
public:
    virtual ~abstract_response() = default;

    response_type type() const;
    status_code status() const;

protected:
    abstract_response(response_type type, status_code status);

private:
    const response_type m_type;
    const status_code m_status;

    friend class response_factory;  // To allow object creation only from the factory
};

}  // namespace server
