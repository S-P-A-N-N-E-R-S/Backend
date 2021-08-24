#include <persistence/database_wrapper.hpp>

namespace server {

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

void database_wrapper::check_connection()
{
    if (!(m_database_connection.is_open()))
    {
        m_database_connection = pqxx::connection(m_connection_string);
    }
}

int database_wrapper::add_job(int user_id, const binary_data &data)
{
    check_connection();
    pqxx::work txn{m_database_connection};

    pqxx::row row_job =
        txn.exec_params1("INSERT INTO jobs (user_id) VALUES ($1) RETURNING job_id", user_id);
    int job_id;
    if (!(row_job[0] >> job_id))
    {
        throw std::runtime_error("Can't access job_id");
    }

    pqxx::row row_request =
        txn.exec_params1("INSERT INTO data (binary_data) VALUES ($1) RETURNING data_id", data);

    int request_id;
    if (!(row_request[0] >> request_id))
    {
        throw std::runtime_error("Can't access request_id");
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

void database_wrapper::add_response(int job_id, const graphs::ResponseContainer &response,
                                    long ogdf_time)
{
    binary_data binary;
    binary.resize(response.ByteSizeLong());
    response.SerializeToArray(binary.data(), binary.size());

    check_connection();

    pqxx::work txn{m_database_connection};

    pqxx::row row_res =
        txn.exec_params1("INSERT INTO data (binary_data) VALUES ($1) RETURNING data_id", binary);

    int result_id;
    if (!(row_res[0] >> result_id))
    {
        throw std::runtime_error("Can't access result_id");
    }

    // If returned number of rows is zero, then the job does no longer exist, an error is thrown and we wont commit
    txn.exec_params1(
        "UPDATE jobs SET end_time = now(), ogdf_runtime = $1, status = 'Success', response_id = $2 "
        "WHERE job_id = $3 RETURNING job_id",
        ogdf_time, result_id, job_id);

    txn.commit();
}

graphs::RequestContainer database_wrapper::get_request_data(int job_id, int user_id)
{
    check_connection();

    pqxx::work txn{m_database_connection};

    pqxx::row row = txn.exec_params1("SELECT binary_data FROM data WHERE data_id = (SELECT "
                                     "request_id FROM jobs WHERE job_id = $1 AND user_id = $2)",
                                     job_id, user_id);

    auto binary = row[0].as<binary_data>();

    auto request_container = graphs::RequestContainer();
    if (request_container.ParseFromArray(binary.data(), binary.size()))
    {
        return request_container;
    }
    else
    {
        throw std::runtime_error("Could not parse protobuff from request!");
    }
}

graphs::ResponseContainer database_wrapper::get_response_data(int job_id, int user_id)
{
    check_connection();

    pqxx::work txn{m_database_connection};

    pqxx::row row = txn.exec_params1("SELECT binary_data FROM data WHERE data_id = (SELECT "
                                     "response_id FROM jobs WHERE job_id = $1 AND user_id = $2)",
                                     job_id, user_id);

    auto binary = row[0].as<binary_data>();

    auto response_container = graphs::ResponseContainer();
    if (response_container.ParseFromArray(binary.data(), binary.size()))
    {
        return response_container;
    }
    else
    {
        throw std::runtime_error("Could not parse protobuff from request!");
    }
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
            throw std::runtime_error("Cant access row!");
        }

        available[i++] = job;
    }

    return available;
}

void database_wrapper::set_started(int job_id)
{
    check_connection();

    pqxx::work txn{m_database_connection};

    txn.exec_params1("UPDATE jobs SET starting_time = now(), status = 'running' WHERE job_id = $1 "
                     "RETURNING job_id",
                     job_id);

    txn.commit();
}

void database_wrapper::set_messages(int job_id, const db_status_type status, const std::string &out,
                                    const std::string &err)
{
    check_connection();

    pqxx::work txn{m_database_connection};

    txn.exec_params1("UPDATE jobs SET status = $1, stdout_msg = $2, error_msg = $3 WHERE job_id = "
                     "$4 RETURNING job_id",
                     status_to_string(status), out, err, job_id);

    txn.commit();
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