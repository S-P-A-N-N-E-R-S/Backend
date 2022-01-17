#include "subcommands/job.hpp"

#include <iostream>

#include <boost/json.hpp>

#include "io/io.hpp"
#include "subcommands/constants.hpp"
#include "util/join.hpp"
#include "util/json.hpp"
#include "util/span.hpp"

namespace cli {

namespace {
    namespace detail {
        // TODO: Might have to add a limit here?
        parse_result_t fetch_jobs()
        {
            auto req = [] {
                boost::json::object req;
                req["type"] = "job";
                req["cmd"] = "list";

                return req;
            }();

            io::instance().send(std::move(req));
            auto resp = io::instance().receive();

            return resp;
        }

        parse_result_t fetch_job_info(std::string_view name_or_id)
        {
            auto req = [name_or_id] {
                boost::json::object req;
                req["type"] = "job";
                req["cmd"] = "info";
                req["arg"] = std::string{name_or_id};

                return req;
            }();

            io::instance().send(std::move(req));
            auto resp = io::instance().receive();

            return resp;
        }

        parse_result_t delete_job(std::string_view name_or_id)
        {
            auto req = [name_or_id] {
                boost::json::object req;
                req["type"] = "job";
                req["cmd"] = "delete";
                req["arg"] = std::string{name_or_id};

                return req;
            }();

            io::instance().send(std::move(req));
            auto resp = io::instance().receive();

            return resp;
        }

        parse_result_t stop_job(std::string_view name_or_id)
        {
            auto req = [name_or_id] {
                boost::json::object req;
                req["type"] = "job";
                req["cmd"] = "stop";
                req["arg"] = std::string{name_or_id};

                return req;
            }();

            io::instance().send(std::move(req));
            auto resp = io::instance().receive();

            return resp;
        }
    }  // namespace detail

    void print_help()
    {
        // clang-format off
        static const std::string_view HELP_TEXT =
            "Available job commands: spannersctl job { delete <job> | stop <job> | list | info <job> }\n"
            "Jobs can be identified by their ID.\n"
            "    delete <job> -- delete the job.\n"
            "    stop <job>   -- stops the job if it is currently running.\n"
            "    list         -- list all jobs.\n"
            "    info <job>   -- print detailed information about a single job.";
        // clang-format on

        std::cout << HELP_TEXT << std::endl;
    }

    exit_code list(span<std::string_view> /* args */)  // TODO: Do we even need args?
    {
        const auto &resp = detail::fetch_jobs();

        if (const auto *error = std::get_if<boost::json::error_code>(&resp); error)
        {
            std::cerr << "Server sent invalid data" << std::endl;
            return exit_code::ERROR;
        }

        // It's not an error so it must be json::value
        const auto msg = std::get<boost::json::value>(resp).as_object();

        if (msg.at("status") != "ok")
        {
            std::cerr << "A server error occurred:\n";
            util::print(std::cerr, msg.at("error"));
            return exit_code::ERROR;
        }

        util::print(std::cout, msg.at("message"));
        return exit_code::OK;
    }

    exit_code info(span<std::string_view> args)
    {
        if (args.empty())
        {
            print_help();
            return exit_code::ERROR;
        }

        const auto name_or_id = util::join(args);
        const auto &resp = detail::fetch_job_info(name_or_id);

        if (const auto *error = std::get_if<boost::json::error_code>(&resp); error)
        {
            std::cerr << "Server sent invalid data" << std::endl;
            return exit_code::ERROR;
        }

        // It's not an error so it must be json::value
        const auto msg = std::get<boost::json::value>(resp).as_object();

        if (msg.at("status") != "ok")
        {
            std::cerr << "A server error occurred:\n";
            util::print(std::cerr, msg.at("error"));
            return exit_code::ERROR;
        }

        util::print(std::cout, msg.at("message"));
        return exit_code::OK;
    }

    // Can't use the language keyword "delete" as function name
    exit_code delete_(span<std::string_view> args)
    {
        if (args.empty())
        {
            print_help();
            return exit_code::ERROR;
        }

        const auto name_or_id = util::join(args);
        const auto &resp = detail::delete_job(name_or_id);

        if (const auto *error = std::get_if<boost::json::error_code>(&resp); error)
        {
            std::cerr << "Server sent invalid data" << std::endl;
            return exit_code::ERROR;
        }

        // It's not an error so it must be json::value
        const auto msg = std::get<boost::json::value>(resp).as_object();

        if (msg.at("status") != "ok")
        {
            std::cerr << "A server error occurred:\n";
            util::print(std::cerr, msg.at("error"));
            return exit_code::ERROR;
        }

        return exit_code::OK;
    }

    exit_code stop(span<std::string_view> args)
    {
        if (args.empty())
        {
            print_help();
            return exit_code::ERROR;
        }

        const auto name_or_id = util::join(args);
        const auto &resp = detail::stop_job(name_or_id);

        if (const auto *error = std::get_if<boost::json::error_code>(&resp); error)
        {
            std::cerr << "Server sent invalid data" << std::endl;
            return exit_code::ERROR;
        }

        // It's not an error so it must be json::value
        const auto msg = std::get<boost::json::value>(resp).as_object();

        if (msg.at("status") != "ok")
        {
            std::cerr << "A server error occurred:\n";
            util::print(std::cerr, msg.at("error"));
            return exit_code::ERROR;
        }

        return exit_code::OK;
    }
}

namespace job {
    exit_code handle(span<std::string_view> args)
    {
        if (args.empty())
        {
            print_help();
            return exit_code::OK;
        }

        const auto &sc = args.front();
        exit_code ec;
        if (sc == "list")
        {
            ec = list(args.tail());
        }
        else if (sc == "info")
        {
            ec = info(args.tail());
        }
        else if (sc == "delete")
        {
            ec = delete_(args.tail());
        }
        else if (sc == "stop")
        {
            ec = stop(args.tail());
        }
        else
        {
            print_help();
            return exit_code::ERROR;
        }

        return ec;
    }
}  // namespace job

}  // namespace cli

