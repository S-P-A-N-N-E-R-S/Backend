#pragma once

#include <boost/json.hpp>
#include <optional>
#include <pqxx/pqxx>
#include <string>
#include <string_view>

#include <networking/messages/meta_data.hpp>
#include <networking/responses/response_factory.hpp>

#include "meta.pb.h"
#include "status.pb.h"

namespace server {

using binary_data = std::basic_string<std::byte>;
using binary_data_view = std::basic_string_view<std::byte>;

// Forward declarations, defined in "persistence/user.hpp"
struct user;
enum class user_role;

/**
 * @brief Struct representing a entry from database table <jobs>
 */
struct job_entry {
    job_entry(const pqxx::row &db_row);

    boost::json::object to_json() const;

    int job_id;
    std::string job_name;
    std::string handler_type;
    int user_id;
    std::string time_received;
    std::string starting_time;
    std::string end_time;
    size_t ogdf_runtime;
    graphs::StatusType status;
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
     * @param status A status.
     */
    void set_status(int job_id, graphs::StatusType status);

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
     * @brief Get the size of the binary_data stored in the data table associated with the job
     *
     * @param job_id  The ID of the job the request belongs to
     * @param user_id The ID of the user the job belongs to
     *
     * @return size_t Sum of the data size
     */
    size_t get_job_data_size(int job_id, int user_id);

    /**
     * @brief Return all available jobs from the database
     *
     * @return std::vector<job_entry> containing all jobs
     */
    std::vector<job_entry> get_all_job_entries();

    /**
     * @brief Resolves a job from the database from a string_view containing either the name or the id
     *
     * @param name_or_id std::string_view containing either job name or the job id
     *
     * @return std::optional<job_entry>
     */
    std::optional<job_entry> resolve_job_entry(std::string_view name_or_id);

    /**
     * @brief Returns job data parsed into struct of type <job_entry>
     *
     * @param job_id  The ID of the job the request belongs to
     * @param user_id The ID of the user the job belongs to
     *
     * @return std::optional<job_entry> Response data parsed as struct of type
     */
    std::optional<job_entry> get_job_entry(int job_id, int user_id);

    /**
     * @brief Returns job data parsed into struct of type <job_entry>
     *
     * @param job_name  The name of the job the request belongs to
     * @param user_id The ID of the user the job belongs to
     *
     * @return std::optional<job_entry> Response data parsed as struct of type
     */
    std::optional<job_entry> get_job_entry(const std::string &job_name, int user_id);

    /**
     * @brief Returns all jobs of a user parsed into structs of type <job_entry>
     *
     * @param user_id The ID of the user the job belongs to
     *
     * @return std::vector<job_entry> containing all jobs of the given user
     */
    std::vector<job_entry> get_job_entries(int user_id);

    /**
     * @brief Deletes a job and stops its execution on the scheduler
     *
     * @param job_id  The ID of the job the request belongs to
     * @param user_id The ID of the user the job belongs to
     *
     * @return true if the job is successfully deleted, else false is returned
     */
    bool delete_job(int job_id, int user_id);

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
    void set_finished(int job_id, graphs::StatusType status, const std::string &out,
                      const std::string &err);

    /**
     * @brief Gets all status information of a job
     *
     * @param job_id
     * @param user_id
     * @return graphs::StatusSingle
     */
    graphs::StatusSingle get_status_data(int job_id, int user_id);

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
     * @brief Gets all existing users from the database
     *
     * @return std::vector<user> containing all existing users
     */
    std::vector<user> get_all_user();

    /**
     * @brief Resolves a user from the database from a string_view containing either the name or the id
     *
     * @param name_or_id std::string_view containing either users name or the users id
     *
     * @return std::optional<user>
     */
    std::optional<user> resolve_user(std::string_view name_or_id);

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
     * @brief Tries to get the user with the corresponding job_id from the database
     *
     * @param job_id int
     * @return std::optional<user> to the user if it exists in database
     */
    std::optional<user> get_user_from_job(int job_id);

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
     * @brief Blocks or unblocks a user to prevent the user from logging in but does not delete its data
     *
     * @param user_id Id of the user in the database
     * @param blocked Bool to indicate the new blocked status of the user
     * @return true if a user with user_id was found, else if not.
     */
    bool block_user(int user_id, bool blocked);

    /**
     * @brief Deletes a user and all of its associated jobs and request/response data
     *
     * @param user_id Id of the user in the database
     * @return true if a user with user_id was found, else if not.
     */
    bool delete_user(int user_id);

    /**
     * @brief Deletes the jobs that belong to a user including their job data
     *
     * @param user_id int representing the ID of the user
     * @return true if a user with the user_id was found, false if not
     */
    bool delete_user_jobs(int user_id);
};

}  // end namespace server
