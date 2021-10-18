#include <persistence/database_wrapper.hpp>

#include "networking/exceptions.hpp"

namespace server {

job_entry::job_entry(const pqxx::row &db_row)
    : job_id{db_row[0].as<int>()}
    , job_name{db_row[1].as<std::string>()}
    , handler_type{db_row[2].as<std::string>()}
    , user_id{db_row[3].as<int>()}
    , ogdf_runtime{db_row[7].as<size_t>()}
    , status{db_row[8].as<std::string>()}
    , stdout_msg{db_row[9].as<std::string>()}
    , error_msg{db_row[10].as<std::string>()}
{
    time_received = (db_row[4].is_null()) ? "" : db_row[4].as<std::string>();
    starting_time = (db_row[5].is_null()) ? "" : db_row[5].as<std::string>();
    end_time = (db_row[6].is_null()) ? "" : db_row[6].as<std::string>();

    request_id = (db_row[11].is_null()) ? -1 : db_row[11].as<int>();
    response_id = (db_row[12].is_null()) ? -1 : db_row[12].as<int>();
}

// Default static connection string
database_wrapper::database_wrapper(const std::string &connection_string)
    : m_connection_string(connection_string)
    , m_database_connection(pqxx::connection(connection_string))
{
}

std::string database_wrapper::status_to_string(db_status_type status)
{
    switch (status)
    {
        case db_status_type::Waiting:
            return "Waiting";
            break;

        case db_status_type::Running:
            return "Running";
            break;

        case db_status_type::Success:
            return "Success";
            break;

        case db_status_type::Failed:
            return "Failed";
            break;

        case db_status_type::Aborted:
            return "Aborted";
            break;

        default:
            return "";
            break;
    }
}

graphs::StatusType database_wrapper::string_to_graphs_status(const std::string &status)
{
    if (status == "Waiting")
    {
        return graphs::StatusType::WAITING;
    }
    else if (status == "Running")
    {
        return graphs::StatusType::RUNNING;
    }
    else if (status == "Success")
    {
        return graphs::StatusType::SUCCESS;
    }
    else if (status == "Failed")
    {
        return graphs::StatusType::FAILED;
    }
    else if (status == "Aborted")
    {
        return graphs::StatusType::ABORTED;
    }
    else
    {
        return graphs::StatusType::UNKNOWN_STATUS;
    }
}

void database_wrapper::check_connection()
{
    if (!(m_database_connection.is_open()))
    {
        m_database_connection = pqxx::connection(m_connection_string);
    }
}

int database_wrapper::add_job(int user_id, const meta_data &meta, binary_data_view data)
{
    check_connection();
    pqxx::work txn{m_database_connection};

    pqxx::row row_job = txn.exec_params1(
        "INSERT INTO jobs (handler_type, job_name, user_id) VALUES ($1, $2, $3) RETURNING job_id",
        meta.handler_type, meta.job_name, user_id);
    int job_id;
    if (!(row_job[0] >> job_id))
    {
        throw row_access_error("Can't access job_id");
    }

    // We don't want to manually maintain an enum in Postgres. Thus, we represent the RequestType as
    // an int in the database.
    pqxx::row row_request =
        txn.exec_params1("INSERT INTO data (type, binary_data) VALUES ($1, $2) RETURNING data_id",
                         static_cast<int>(meta.request_type), data);

    int request_id;
    if (!(row_request[0] >> request_id))
    {
        throw row_access_error("Can't access request_id");
    }

    txn.exec_params0("UPDATE jobs SET request_id = $1 WHERE job_id = $2", request_id, job_id);

    txn.commit();

    return job_id;
}

void database_wrapper::set_status(int job_id, db_status_type status)
{
    check_connection();

    pqxx::work txn{m_database_connection};

    txn.exec_params1("UPDATE jobs SET status = '$1' WHERE job_id = $2 RETURNING job_id",
                     status_to_string(status), job_id);

    txn.commit();
}

void database_wrapper::add_response(int job_id, graphs::RequestType type,
                                    const graphs::ResponseContainer &response, long ogdf_time)
{
    binary_data binary;
    binary.resize(response.ByteSizeLong());
    response.SerializeToArray(binary.data(), binary.size());

    check_connection();

    pqxx::work txn{m_database_connection};

    // We don't want to manually maintain an enum in Postgres. Thus, we represent the RequestType as
    // an int in the database.
    pqxx::row row_res =
        txn.exec_params1("INSERT INTO data (type, binary_data) VALUES ($1, $2) RETURNING data_id",
                         static_cast<int>(type), binary);

    int result_id;
    if (!(row_res[0] >> result_id))
    {
        throw row_access_error("Can't access result_id");
    }

    // If returned number of rows is zero, then the job does no longer exist, an error is thrown and we wont commit
    txn.exec_params1(
        "UPDATE jobs SET ogdf_runtime = $1, response_id = $2 WHERE job_id = $3 RETURNING job_id",
        ogdf_time, result_id, job_id);

    txn.commit();
}

std::pair<graphs::RequestType, graphs::RequestContainer> database_wrapper::get_request_data(
    int job_id, int user_id)
{
    check_connection();

    pqxx::work txn{m_database_connection};

    pqxx::row row = txn.exec_params1("SELECT type, binary_data FROM data WHERE data_id = (SELECT "
                                     "request_id FROM jobs WHERE job_id = $1 AND user_id = $2)",
                                     job_id, user_id);

    const auto type = static_cast<graphs::RequestType>(row[0].as<int>());
    auto binary = row[1].as<binary_data>();

    auto request_container = graphs::RequestContainer();
    if (request_container.ParseFromArray(binary.data(), binary.size()))
    {
        return {type, request_container};
    }
    else
    {
        throw std::runtime_error("Could not parse protobuff from request!");
    }
}

std::pair<graphs::RequestType, graphs::ResponseContainer> database_wrapper::get_response_data(
    int job_id, int user_id)
{
    check_connection();

    pqxx::work txn{m_database_connection};

    pqxx::row row = txn.exec_params1("SELECT type, binary_data FROM data WHERE data_id = (SELECT "
                                     "response_id FROM jobs WHERE job_id = $1 AND user_id = $2)",
                                     job_id, user_id);

    const auto type = static_cast<graphs::RequestType>(row[0].as<int>());
    auto binary = row[1].as<binary_data>();

    auto response_container = graphs::ResponseContainer();
    if (response_container.ParseFromArray(binary.data(), binary.size()))
    {
        return {type, response_container};
    }
    else
    {
        throw std::runtime_error("Could not parse protobuff from request!");
    }
}

std::pair<graphs::RequestType, binary_data> database_wrapper::get_response_data_raw(int job_id,
                                                                                    int user_id)
{
    check_connection();

    pqxx::work txn{m_database_connection};

    pqxx::row row = txn.exec_params1("SELECT type, binary_data FROM data WHERE data_id = (SELECT "
                                     "response_id FROM jobs WHERE job_id = $1 AND user_id = $2)",
                                     job_id, user_id);

    const auto type = static_cast<graphs::RequestType>(row[0].as<int>());
    auto binary = row[1].as<binary_data>();
    return {type, std::move(binary)};
}

job_entry database_wrapper::get_job_entry(int job_id, int user_id)
{
    check_connection();

    pqxx::work txn{m_database_connection};
    pqxx::row row =
        txn.exec_params1("SELECT * FROM jobs WHERE job_id = $1 AND user_id = $2)", job_id, user_id);

    return job_entry{row};
}

std::vector<job_entry> database_wrapper::get_job_entries(int user_id)
{
    check_connection();

    pqxx::work txn{m_database_connection};
    pqxx::result rows = txn.exec_params("SELECT * FROM jobs WHERE user_id = $1", user_id);

    std::vector<job_entry> jobs;
    for (const auto &row : rows)
    {
        jobs.emplace_back(row);
    }

    return jobs;
}

meta_data database_wrapper::get_meta_data(int job_id, int user_id)
{
    check_connection();

    pqxx::work txn{m_database_connection};
    pqxx::row row =
        txn.exec_params1("SELECT type, handler_type, job_name FROM jobs LEFT JOIN data ON "
                         "request_id = data_id WHERE job_id = $1 AND user_id = $2",
                         job_id, user_id);

    return meta_data{(row[0].is_null()) ? graphs::RequestType::UNDEFINED_REQUEST
                                        : static_cast<graphs::RequestType>(row[0].as<int>()),
                     (row[1].is_null()) ? "" : row[1].as<std::string>(),
                     (row[2].is_null()) ? "" : row[2].as<std::string>()};
}

std::vector<std::pair<int, int>> database_wrapper::get_next_jobs(int n)
{
    check_connection();

    pqxx::work txn{m_database_connection};

    pqxx::result rows = txn.exec_params("SELECT job_id, user_id FROM jobs WHERE STATUS = 'waiting' "
                                        "ORDER BY time_received ASC LIMIT $1",
                                        n);

    std::vector<std::pair<int, int>> available(rows.size());

    size_t i = 0;

    for (auto const &row : rows)
    {
        std::pair<int, int> job;

        if (!(row[0] >> job.first && row[1] >> job.second))
        {
            throw row_access_error("Can't access row", rows);
        }

        available[i++] = job;
    }

    return available;
}

void database_wrapper::set_started(int job_id)
{
    check_connection();

    pqxx::work txn{m_database_connection};

    txn.exec_params1("UPDATE jobs SET starting_time = now(), status = 'Running' WHERE job_id = $1 "
                     "RETURNING job_id",
                     job_id);

    txn.commit();
}

void database_wrapper::set_finished(int job_id, const db_status_type status, const std::string &out,
                                    const std::string &err)
{
    check_connection();

    pqxx::work txn{m_database_connection};

    txn.exec_params1("UPDATE jobs SET status = $1, end_time=now(), stdout_msg = $2, error_msg = $3 "
                     " WHERE job_id = $4 RETURNING job_id",
                     status_to_string(status), out, err, job_id);

    txn.commit();
}

std::string database_wrapper::get_status(int job_id, int user_id)
{
    check_connection();

    pqxx::work txn{m_database_connection};

    pqxx::result rows = txn.exec_params(
        "SELECT status FROM jobs WHERE job_id = $1 AND user_id = $2", job_id, user_id);

    std::string status;

    if (rows.size() < 1)
    {
        status = "Not Found";
    }
    else if (!(rows[0][0] >> status))
    {
        throw row_access_error("Can't access row", rows);
    }

    return status;
}

std::vector<std::pair<int, std::string>> database_wrapper::get_status(int user_id)
{
    check_connection();

    pqxx::work txn{m_database_connection};

    pqxx::result rows = txn.exec_params("SELECT job_id, status FROM jobs WHERE user_id = $1 "
                                        "ORDER BY job_id",
                                        user_id);

    std::vector<std::pair<int, std::string>> states(rows.size());

    size_t i = 0;

    for (auto const &row : rows)
    {
        std::pair<int, std::string> status_info;

        if (!(row[0] >> status_info.first && row[1] >> status_info.second))
        {
            throw row_access_error("Can't access row", rows);
        }

        states[i++] = std::move(status_info);
    }

    return states;
}

// /**
//  * For Dispatcher <-> Persistence communication (user side).
//  * Lists job according to a filter (i.e. user, status, date, ...). Only Metadata are returned
//  * @param filter a filter for the jobs
//  * @returns list of the jobs
//  */
// JobMetadataContainer[] get_jobs(FilterInterface filter);
// TODO: Not implementet yet, not needed yet. Define filter functions.

}  // namespace server
