#pragma once

#include <optional>
#include <pqxx/pqxx>
#include <string>

#include <networking/messages/meta_data.hpp>
#include <networking/responses/response_factory.hpp>
#include <persistence/user.hpp>

#include "meta.pb.h"
#include "status.pb.h"

namespace server {

using binary_data = std::basic_string<std::byte>;
using binary_data_view = std::basic_string_view<std::byte>;

enum class db_status_type { Waiting, Running, Success, Failed, Aborted };

/**
 * @brief Struct representing a entry from database table <jobs>
 */
struct job_entry {
    job_entry(const pqxx::row &db_row);

    int job_id;
    std::string job_name;
    std::string handler_type;
    int user_id;
    std::string time_received;
    std::string starting_time;
    std::string end_time;
    size_t ogdf_runtime;
    std::string status;
    std::string stdout_msg;
    std::string error_msg;
    int request_id;
    int response_id;
};

class database_wrapper
{
private:
    const std::string m_connection_string;
    pqxx::connection m_database_connection;
    void check_connection();

public:
    database_wrapper(const std::string &connection_string);

    /**
     * Adds the parsed binary data of a request to the database.
     *
     * @param user_id The ID of the user who scheduled the job
     * @param type    The type of the request (defined in the accompanying meta message)
     * @param handler_type String to identify the handler that is used to execute the job
     * @param binary  View to binary data that contains the parsed request
     *
     * @return ID of the inserted job
     */
    int add_job(int user_id, const meta_data &meta, binary_data_view binary);

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
     * @brief Returns the unparsed data of a finished job's response from the database
     *
     * @param job_id  The ID of the job the request belongs to
     * @param user_id The ID of the user the job belongs to
     *
     * @return A pair of the original RequestType and the binary_data.
     */
    std::pair<graphs::RequestType, binary_data> get_response_data_raw(int job_id, int user_id);

    /**
     * @brief Returns job data parsed into struct of type <job_entry>
     *
     * @param job_id  The ID of the job the request belongs to
     * @param user_id The ID of the user the job belongs to
     *
     * @return Response data parsed as struct of type <job_entry>
     */
    job_entry get_job_entry(int job_id, int user_id);

    /**
     * @brief Returns all jobs of a user parsed into structs of type <job_entry>
     *
     * @param user_id The ID of the user the job belongs to
     *
     * @return std::vector<job_entry> containing all jobs of the given user
     */
    std::vector<job_entry> get_job_entries(int user_id);

    /**
     * @brief Returns the meta information of a job
     *
     * @param job_id  The ID of the job the request belongs to
     * @param user_id The ID of the user the job belongs to
     *
     * @return struct server::meta_data
     */
    meta_data get_meta_data(int job_id, int user_id);

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
     * @brief Notifies databse that a job is finished (regardless if successfull or not.).
     * Sets status and both messages of a job, end_time is set to now()
     *
     * @param job_id id of the job
     * @param status The new status
     * @param out New entry of field stdout_msg
     * @param err New entry of field error_message
     */
    void set_finished(int job_id, const db_status_type status, const std::string &out,
                      const std::string &err);

    /**
     * @brief Gets the status of a single job of a user
     */
    std::string get_status(int job_id, int user_id);

    /**
     * @brief Gets the status for all jobs of a user
     *
     * @param user_id
     * @return list of jobs and their respective states
     */
    std::vector<std::pair<int, std::string>> get_status(int user_id);

    /**
     * @brief Converts a db_Status_type to a string
     *
     * @param status
     * @return status as string
     */
    static std::string status_to_string(db_status_type status);

    /**
     * @brief Converts a string to a graphs::StatusType
     *
     * @param status
     * @return status as graphs::StatusType
     */
    static graphs::StatusType string_to_graphs_status(const std::string &status);

    /**
     * @brief Tries to create the user u in the database and writes its database-id into
     * the field user_id.
     * 
     * @param u the user to create. The user_id field is ignored for creation and overwritten
     * with the new id
     * @return true if user was created in database, false if a user with the username already 
     * exists
     */
    bool create_user(user &u);

    /**
     * @brief Tries to get the user with the corresponding user_name from the database
     * 
     * @param name 
     * @return std::optional<user> to the user if it exists in database
     */
    std::optional<user> get_user(const std::string &name);

    /**
     * @brief Tries to get the user with the corresponding user_id from the database
     * 
     * @param name 
     * @return std::optional<user> to the user if it exists in database
     */
    std::optional<user> get_user(int user_id);

    /**
     * @brief Changes the hashed password and the salt in the database.
     * 
     * @param user_id Id of the user in the database
     * @param pw_new The hash of the new password together with the salt
     * @param salt_new The new salt
     * @return true if a user with user_id was found, else if not.
     */
    bool change_user_auth(int user_id, const std::string &pw_hash, const std::string &salt);

    /**
     * @brief Changes the role of a user in the database.
     * 
     * @param user_id Id of the user in the database
     * @param role New role of the user.
     * @return true if a user with user_id was found, else if not.
     */
    bool change_user_role(int user_id, user_role role);

    /**
     * @brief Deletes a user and all of its associated jobs and request/response data
     * 
     * @param user_id Id of the user in the database
     * @return true if a user with user_id was found, else if not.
     */
    bool delete_user(int user_id);
};

}  // end namespace server
