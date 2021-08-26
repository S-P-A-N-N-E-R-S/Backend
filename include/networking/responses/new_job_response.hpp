#pragma once

#include "networking/responses/abstract_response.hpp"

#include "new_job_response.pb.h"

namespace server {

class new_job_response : public abstract_response
{
public:
    new_job_response(graphs::NewJobResponse &&new_job_response, status_code status);
    virtual ~new_job_response() = default;

private:
    graphs::NewJobResponse m_new_job_response;
};

}  // namespace server
