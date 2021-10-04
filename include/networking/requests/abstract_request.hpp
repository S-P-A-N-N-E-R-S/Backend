#pragma once

#include "networking/requests/request_factory.hpp"
#include "networking/requests/request_type.hpp"

namespace server {

class abstract_request
{
public:
    virtual ~abstract_request() = default;

    request_type type() const;

protected:
    abstract_request(request_type type);
    const request_type m_type;

private:
    // To allow object creation only from the factory
    friend std::unique_ptr<abstract_request> request_factory::build_request(
        graphs::RequestType, const graphs::RequestContainer &);
};

}  // namespace server
