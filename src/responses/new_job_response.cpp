#include "networking/responses/new_job_response.hpp"

namespace server {

new_job_response::new_job_response(graphs::NewJobResponse &&new_job_response, status_code status)
    : abstract_response{response_type::NEW_JOB, status}
    , m_new_job_response{new_job_response}
{
}

}  // namespace server
