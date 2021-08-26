#pragma once

#include "networking/responses/abstract_response.hpp"
#include "status.pb.h"

namespace server {

class status_response : public abstract_response
{
public:
    status_response(graphs::StatusResponse &&statusResponse, status_code status);
    virtual ~status_response() = default;

    graphs::StatusResponse take_status_response();

private:
    graphs::StatusResponse m_statusResponse;
};

}  // namespace server
