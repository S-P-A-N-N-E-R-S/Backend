#pragma once

#include "networking/requests/request_type.hpp"

#include "GraphData.pb.h"

namespace server {

class abstract_request
{
public:
    virtual ~abstract_request() = default;

    request_type type() const;

protected:
    abstract_request(request_type type);

private:
    const request_type m_type;

    friend class request_factory;  // To allow object creation only from the factory
};

}  // namespace server
