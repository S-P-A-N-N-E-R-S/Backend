#include <persistence/database_wrapper.hpp>
#include <scheduler/scheduler.hpp>

#include "networking/exceptions.hpp"
#include "networking/utils.hpp"

#include <google/protobuf/util/time_util.h>

namespace server {

job_entry::job_entry(const pqxx::row &db_row)
    : job_id{db_row[0].as<int>()}
    , job_name{db_row[1].as<std::string>()}
    , handler_type{db_row[2].as<std::string>()}
    , user_id{db_row[3].as<int>()}
    , ogdf_runtime{db_row[7].as<size_t>()}
    , status{db_row[8].as<int>()}
    , stdout_msg{db_row[9].as<std::string>()}
    , error_msg{db_row[10].as<std::string>()}
{
    if (!db_row[4].is_null())
    {
        time_received = db_row[4].as<std::string>();
        utils::pqxx_timestampz_to_iso8601(time_received);
    }

    if (!db_row[5].is_null())
    {
        starting_time = db_row[5].as<std::string>();
        utils::pqxx_timestampz_to_iso8601(starting_time);
    }

    if (!db_row[6].is_null())
    {
        end_time = db_row[6].as<std::string>();
        utils::pqxx_timestampz_to_iso8601(end_time);
    }

    request_id = (db_row[11].is_null()) ? -1 : db_row[11].as<int>();
    response_id = (db_row[12].is_null()) ? -1 : db_row[12].as<int>();
}

// Default static connection string
database_wrapper::database_wrapper(const std::string &connection_string)
    : m_connection_string(connection_string)
    , m_database_connection(pqxx::connection(connection_string))
{
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

    pqxx::row row_job = txn.exec_params1("INSERT INTO jobs (handler_type, job_name, user_id, "
                                         "status) VALUES ($1, $2, $3, $4) RETURNING job_id",
                                         meta.handler_type, meta.job_name, user_id,
                                         static_cast<int>(graphs::StatusType::WAITING));
    int job_id;
    if (!(row_job[0] >> job_id))
    {
        throw row_access_error("Can't access job_id");
    }

    // We don't want to manually maintain an enum in Postgres. Thus, we represent the RequestType as
    // an int in the database.
    pqxx::row row_request = txn.exec_params1(
        "INSERT INTO data (job_id, type, binary_data) VALUES ($1, $2, $3) RETURNING data_id",
        job_id, static_cast<int>(meta.request_type), data);

    int request_id;
    if (!(row_request[0] >> request_id))
    {
        throw row_access_error("Can't access request_id");
    }

    txn.exec_params0("UPDATE jobs SET request_id = $1 WHERE job_id = $2", request_id, job_id);

    txn.commit();

    return job_id;
}

void database_wrapper::set_status(int job_id, graphs::StatusType status)
{
    check_connection();

    pqxx::work txn{m_database_connection};

    txn.exec_params1("UPDATE jobs SET status = '$1' WHERE job_id = $2 RETURNING job_id",
                     static_cast<int>(status), job_id);

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
    pqxx::row row_res = txn.exec_params1(
        "INSERT INTO data (job_id, type, binary_data) VALUES ($1, $2, $3) RETURNING data_id",
        job_id, static_cast<int>(type), binary);

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
        txn.exec_params1("SELECT * FROM jobs WHERE job_id = $1 AND user_id = $2", job_id, user_id);

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
                         "request_id = data_id WHERE jobs.job_id = $1 AND user_id = $2",
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

    pqxx::result rows = txn.exec_params("SELECT job_id, user_id FROM jobs WHERE STATUS = $1 "
                                        "ORDER BY time_received ASC LIMIT $2",
                                        static_cast<int>(graphs::StatusType::WAITING), n);

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

    txn.exec_params1("UPDATE jobs SET starting_time = now(), status = $1 WHERE job_id = $2 "
                     "RETURNING job_id",
                     static_cast<int>(graphs::StatusType::RUNNING), job_id);

    txn.commit();
}

void database_wrapper::set_finished(int job_id, graphs::StatusType status, const std::string &out,
                                    const std::string &err)
{
    check_connection();

    pqxx::work txn{m_database_connection};

    txn.exec_params1("UPDATE jobs SET status = $1, end_time=now(), stdout_msg = $2, error_msg = $3 "
                     " WHERE job_id = $4 RETURNING job_id",
                     static_cast<int>(status), out, err, job_id);

    txn.commit();
}

graphs::StatusSingle database_wrapper::get_status_data(int job_id, int user_id)
{
    graphs::StatusSingle status_single;

    // Use meta data of the job to get the request type
    job_entry job = get_job_entry(job_id, user_id);
    meta_data job_meta_data = get_meta_data(job.job_id, user_id);

    status_single.set_job_id(job.job_id);
    status_single.set_status(job.status);
    status_single.set_statusmessage(std::move(job.error_msg));
    status_single.set_requesttype(job_meta_data.request_type);
    status_single.set_handlertype(std::move(job.handler_type));
    status_single.set_jobname(std::move(job.job_name));
    status_single.set_ogdfruntime(job.ogdf_runtime);

    if (job.time_received != "")
    {
        google::protobuf::util::TimeUtil::FromString(job.time_received,
                                                     status_single.mutable_timereceived());
    }
    if (job.starting_time != "")
    {
        google::protobuf::util::TimeUtil::FromString(job.starting_time,
                                                     status_single.mutable_startingtime());
    }
    if (job.end_time != "")
    {
        google::protobuf::util::TimeUtil::FromString(job.end_time, status_single.mutable_endtime());
    }

    return status_single;
}

bool database_wrapper::create_user(user &u)
{
    check_connection();
    pqxx::work txn{m_database_connection};

    // pqxx prepared statements protect from sql injections, so no escaping needed here
    // (https://libpqxx.readthedocs.io/en/stable/a01383.html)

    pqxx::result rows = txn.exec_params("SELECT user_id FROM users WHERE user_name = $1", u.name);

    if (rows.size() != 0)
    {
        return false;
    }

    auto row_request = txn.exec_params1("INSERT INTO users (user_name, pw_hash, salt, role) VALUES "
                                        "($1, $2, $3, $4) RETURNING user_id",
                                        u.name, u.pw_hash, u.salt, static_cast<int>(u.role));

    if (!(row_request[0] >> u.user_id))
    {
        throw row_access_error("Can't access user_id");
    }

    txn.commit();

    return true;
}

std::optional<user> database_wrapper::get_user(const std::string &name)
{
    check_connection();

    // pqxx prepared statements protect from sql injections, so no escaping needed here
    // (https://libpqxx.readthedocs.io/en/stable/a01383.html)

    pqxx::work txn{m_database_connection};
    pqxx::result result = txn.exec_params(
        "SELECT user_id, user_name, pw_hash, salt, role FROM users WHERE user_name = $1", name);

    if (result.size() != 1)
    {
        return std::nullopt;
    }

    pqxx::row row = result[0];
    return std::optional<user>{user::from_row(row)};
}

std::optional<user> database_wrapper::get_user(int user_id)
{
    check_connection();

    pqxx::work txn{m_database_connection};
    pqxx::result result = txn.exec_params(
        "SELECT user_id, user_name, pw_hash, salt, role FROM users WHERE user_id = $1", user_id);

    if (result.size() != 1)
    {
        return std::nullopt;
    }

    pqxx::row row = result[0];
    return std::optional<user>{user::from_row(row)};
}

bool database_wrapper::change_user_role(int user_id, user_role role)
{
    check_connection();

    pqxx::work txn{m_database_connection};

    pqxx::result result =
        txn.exec_params("UPDATE users SET role=$1 WHERE user_id = $2 RETURNING user_id",
                        static_cast<int>(role), user_id);

    if (result.size() != 1)
    {
        return false;
    }

    txn.commit();
    return true;
}

bool database_wrapper::change_user_auth(int user_id, const std::string &pw_hash,
                                        const std::string &salt)
{
    check_connection();

    pqxx::work txn{m_database_connection};

    pqxx::result result =
        txn.exec_params("UPDATE users SET pw_hash=$1, salt=$2 WHERE user_id = $3 RETURNING user_id",
                        pw_hash, salt, user_id);

    if (result.size() != 1)
    {
        return false;
    }

    txn.commit();
    return true;
}

bool database_wrapper::delete_user(int user_id)
{
    check_connection();

    pqxx::work txn{m_database_connection};

    pqxx::result rows = txn.exec_params("SELECT user_id FROM users WHERE user_name = $1", user_id);

    if (rows.size() != 0)
    {
        return false;
    }

    //empty the queue of user jobs
    txn.exec_params0("UPDATE jobs SET status=$1 WHERE user_id=$2 AND status=$3",
                     static_cast<int>(graphs::StatusType::ABORTED), user_id,
                     static_cast<int>(graphs::StatusType::WAITING));

    txn.commit();

    scheduler::instance().cancel_user_jobs(user_id);

    pqxx::work txn2{m_database_connection};

    txn2.exec_params0("DELETE FROM users WHERE user_id=$1", user_id);

    txn2.commit();
    return true;
}

}  // namespace server
