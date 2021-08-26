#pragma once

#include <pqxx/pqxx>
#include <string>

#include <networking/responses/response_factory.hpp>

#include "meta.pb.h"

typedef std::basic_string<std::byte> binary_data;

namespace server {

enum class db_status_type { Waiting, Running, Success, Failed, Aborted };

class database_wrapper
{
private:
    const std::string m_connection_string;
    pqxx::connection m_database_connection;
    void check_connection();

    std::string status_to_string(db_status_type status);

public:
    database_wrapper(const std::string &connection_string);

    /**
     * Adds the parsed binary data of a request to the database.
     *
     * @param user_id The ID of the user who scheduled the job
     * @param type    The type of the request (defined in the accompanying meta message)
     * @param binary  A binary data object that contains the parsed request
     *
     * @return ID of the inserted job
     */
    int add_job(int user_id, graphs::RequestType type, const binary_data &binary);

    /**
     * Sets the status of a job to 'waiting', 'in progress', 'finished' or 'aborted'.
     * @param job_id The ID of the job where the status should be changed.
     * @param status A status string.
     */
    void set_status(int job_id, const db_status_type status);

    /**
     * Adds the result of a request parsed as binary data to the database.
     *
     * @param job_id    The ID of the job where the result should be changed
     * @param type      The type of the response (will be included in the accompanying meta message)
     * @param response  A container that contains the parsed response message
     * @param ogdf_time Runtime of the ogdf call in microseconds
     */
    void add_response(int job_id, graphs::RequestType type,
                      const graphs::ResponseContainer &response, long ogdf_time);

    /**
     * Reads the parsed data of a request from the database.
     *
     * @param job_id  The ID of the job the request belongs to
     * @param user_id The ID of the user the job belongs to
     *
     * @return A pair of the original RequestType and the RequestContainer.
     */
    std::pair<graphs::RequestType, graphs::RequestContainer> get_request_data(int job_id,
                                                                              int user_id);

    /**
     * Reads the parsed data of a finished job's response from the database.
     *
     * @param job_id  The ID of the job the request belongs to
     * @param user_id The ID of the user the job belongs to
     *
     * @return A pair of the original RequestType and the ResponseContainer.
     */
    std::pair<graphs::RequestType, graphs::ResponseContainer> get_response_data(int job_id,
                                                                                int user_id);

    /**
     * Retrieves a list of the next available jobs from the database. Ordered by time the job was queued.
     * @param n Number of next jobs.
     * @return An ordered list of the next available, not yet started, jobs. First is job_id, second is user_id
     */
    std::vector<std::pair<int, int>> get_next_jobs(int n);

    /**
     * Updates the starting date of a job to the current date.
     * @param job_id The ID of the job entry that should be changed.
     */
    void set_started(int job_id);

    /**
     * @brief Sets status and both messages of a job
     * 
     * @param job_id id of the job
     * @param status The new status
     * @param out New entry of field stdout_msg
     * @param err New entry of field error_message
     */
    void set_messages(int job_id, const db_status_type status, const std::string &out,
                      const std::string &err);
};

}  // end namespace server
