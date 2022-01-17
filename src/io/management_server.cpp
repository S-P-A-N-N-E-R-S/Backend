#include <networking/io/management_server.hpp>

#include <boost/system/error_code.hpp>
#include <iostream>
#include <stdexcept>
#include <string>

#include <config/config.hpp>
#include <persistence/database_wrapper.hpp>
#include <persistence/user.hpp>
#include <scheduler/scheduler.hpp>

#include "status.pb.h"

using boost::asio::buffer;
using boost::asio::yield_context;
using boost::asio::local::datagram_protocol;
using boost::program_options::variable_value;
using boost::system::error_code;
using graphs::StatusType;

namespace json = boost::json;

namespace server {

namespace {

    json::object handle_user_cmd(std::string_view cmd, const json::value &arg)
    {
        database_wrapper db{get_db_connection_string()};

        json::object message{};
        if (cmd == "delete")
        {
            // Resolve user
            std::string_view name_or_id = arg.as_string();
            std::optional<user> user = db.resolve_user(name_or_id);
            if (!user)
            {
                throw std::out_of_range{"User not found"};
            }

            db.delete_user_jobs(user->user_id);
            db.delete_user(user->user_id);
        }
        else if (cmd == "block")
        {
            // Resolve user
            std::string_view name_or_id = arg.as_string();
            std::optional<user> user = db.resolve_user(name_or_id);
            if (!user)
            {
                throw std::out_of_range{"User not found"};
            }

            db.block_user(user->user_id, true);
        }
        else if (cmd == "list")
        {
            json::array user_list{};
            for (const auto &user : db.get_all_user())
            {
                user_list.push_back(user.to_json());
            }
            message["user"] = std::move(user_list);
        }
        else if (cmd == "info")
        {
            // Resolve user
            std::string_view name_or_id = arg.as_string();
            std::optional<user> user = db.resolve_user(name_or_id);
            if (!user)
            {
                throw std::out_of_range{"User not found"};
            }

            message = user->to_json();

            json::array json_jobs{};
            for (const auto &job : db.get_job_entries(user->user_id))
            {
                json::object j_job = job.to_json();
                j_job["data_size"] = db.get_job_data_size(job.job_id, user->user_id);
                json_jobs.push_back(std::move(j_job));
            }
            message["jobs"] = std::move(json_jobs);
        }
        else
        {
            throw std::out_of_range{"Invalid cmd"};
        }

        return message;
    }

    json::object handle_job_cmd(std::string_view cmd, const json::value &arg)
    {
        database_wrapper db{get_db_connection_string()};

        json::object message{};
        if (cmd == "delete")
        {
            // Resolve job
            std::string_view name_or_id = arg.as_string();
            std::optional<job_entry> job = db.resolve_job_entry(name_or_id);
            if (!job)
            {
                throw std::out_of_range{"Job not found"};
            }

            db.delete_job(job->job_id, job->user_id);
        }
        else if (cmd == "stop")
        {
            // Resolve job
            std::string_view name_or_id = arg.as_string();
            std::optional<job_entry> job = db.resolve_job_entry(name_or_id);
            if (!job)
            {
                throw std::out_of_range{"Job not found"};
            }

            scheduler::instance().cancel_job(job->job_id, job->user_id);
        }
        else if (cmd == "list")
        {
            json::array json_jobs{};
            for (const auto &job : db.get_all_job_entries())
            {
                json::object j_job = job.to_json();
                j_job.erase("stdout");
                j_job.erase("error");
                j_job["data_size"] = db.get_job_data_size(job.job_id, job.user_id);
                json_jobs.push_back(std::move(j_job));
            }
            message["jobs"] = std::move(json_jobs);
        }
        else if (cmd == "info")
        {
            // Resolve job
            std::string_view name_or_id = arg.as_string();
            std::optional<job_entry> job = db.resolve_job_entry(name_or_id);
            if (!job)
            {
                throw std::out_of_range{"Job not found"};
            }

            message = job->to_json();
            message["data_size"] = db.get_job_data_size(job->job_id, job->user_id);
        }
        else
        {
            throw std::out_of_range{"Invalid cmd"};
        }

        return message;
    }

    json::object handle_scheduler_cmd(std::string_view cmd, const json::value &arg)
    {
        json::object message{};
        if (cmd == "time-limit")
        {
            if (arg.is_int64())
            {
                scheduler::instance().set_time_limit(arg.as_int64());
                modify_config(config_options::SCHEDULER_TIME_LIMIT,
                              variable_value{arg.as_int64(), true});
            }
            message["time-limit"] = scheduler::instance().get_time_limit();
        }
        else if (cmd == "resource-limit")
        {
            if (arg.is_int64())
            {
                scheduler::instance().set_resource_limit(arg.as_int64());
                modify_config(config_options::SCHEDULER_RESOURCE_LIMIT,
                              variable_value{arg.as_int64(), true});
            }
            message["resource-limit"] = scheduler::instance().get_resource_limit();
        }
        else if (cmd == "process-limit")
        {
            if (arg.is_int64())
            {
                int64_t process_limit = arg.as_int64();
                if (process_limit <= 0)
                {
                    throw std::invalid_argument{"Invalid value for process-limit provided"};
                }
                scheduler::instance().set_process_limit(arg.as_int64());
                modify_config(config_options::SCHEDULER_PROCESS_LIMIT,
                              variable_value{static_cast<size_t>(arg.as_int64()), true});
            }
            message["process-limit"] = scheduler::instance().get_process_limit();
        }
        else if (cmd == "sleep")
        {
            if (arg.is_int64())
            {
                scheduler::instance().set_sleep(arg.as_int64());
                modify_config(config_options::SCHEDULER_SLEEP,
                              variable_value{arg.as_int64(), true});
            }
            message["sleep"] = scheduler::instance().get_sleep();
        }
        else
        {
            throw std::out_of_range{"Invalid cmd"};
        }

        return message;
    }

}  // namespace

management_server::management_server(std::string_view descriptor)
    : io_server{}
    , m_sock{[this, descriptor] {
        // Make sure the local socket is unlinked before usage
        ::unlink(descriptor.data());
        return datagram_protocol::socket{m_ctx, datagram_protocol::endpoint{descriptor.data()}};
    }()}
{
}

void management_server::handle()
{
    boost::asio::spawn(m_ctx, [this](yield_context yield) {
        std::cout << "[INFO] Management API listening on " << m_sock.local_endpoint().path()
                  << '\n';

        while (m_status == RUNNING)
        {
            error_code err;
            m_sock.async_wait(datagram_protocol::socket::wait_read, yield[err]);
            if (!err)
            {
                auto [sender, request] = read_json(yield);
                boost::asio::spawn(m_ctx, [this, sender = std::move(sender),
                                           request = std::move(request)](yield_context yield) {
                    try
                    {
                        handle_request(yield, sender, std::move(request.as_object()));
                    }
                    catch (const std::out_of_range &e)
                    {
                        // Thrown from boost::json when key does not exists - indicates malformed request
                        json::object response;
                        response["status"] = "invalid-request-error";
                        response["error"] = e.what();
                        respond_json(yield, sender, response);
                    }
                    catch (const std::invalid_argument &e)
                    {
                        json::object response;
                        response["status"] = "invalid-argument-error";
                        response["error"] = e.what();
                        respond_json(yield, sender, response);
                    }
                    catch (const std::exception &e)
                    {
                        json::object response;
                        response["status"] = "internal-error";
                        response["error"] = e.what();
                        respond_json(yield, sender, response);
                    }
                });
            }
        }
    });
}

void management_server::handle_request(yield_context &yield,
                                       const datagram_protocol::endpoint &sender,
                                       json::object request)
{
    //  TODO Update values at boost::program_options::variables_map in config

    json::object response{};
    std::string_view request_type = request.at("type").as_string();
    if (request_type == "user")
    {
        response["message"] = handle_user_cmd(request.at("cmd").as_string(), request["arg"]);
    }
    else if (request_type == "job")
    {
        response["message"] = handle_job_cmd(request.at("cmd").as_string(), request["arg"]);
    }
    else if (request_type == "scheduler")
    {
        response["message"] = handle_scheduler_cmd(request.at("cmd").as_string(), request["arg"]);
    }

    response["status"] = "ok";
    respond_json(yield, sender, response);
}

std::pair<datagram_protocol::endpoint, json::value> management_server::read_json(
    yield_context &yield)
{
    std::vector<char> recv_buffer(m_sock.available());
    error_code err;
    datagram_protocol::endpoint sender;
    m_sock.async_receive_from(buffer(recv_buffer.data(), recv_buffer.size()), sender, yield[err]);
    if (err)
    {
        throw std::runtime_error{"Failed to receive data"};
    }

    json::value request =
        json::parse(json::string_view{recv_buffer.data(), recv_buffer.size()}, err);
    if (err)
    {
        throw std::runtime_error{"Failed to parse request into valid json"};
    }

    return std::make_pair(sender, request);
}

void management_server::respond_json(yield_context &yield,
                                     const datagram_protocol::endpoint &receiver,
                                     const json::object &response)
{
    std::string serialized = json::serialize(response);
    error_code err;
    m_sock.async_send_to(buffer(serialized.data(), serialized.size()), receiver, yield[err]);
    if (err)
    {
        throw std::runtime_error{"Failed to send data"};
    }
}

}  // namespace server
