#include "subcommands/scheduler.hpp"

#include <charconv>
#include <iostream>
#include <optional>
#include <stdexcept>

#include <sys/resource.h>

#include <nlohmann/json.hpp>

#include "io/io.hpp"
#include "subcommands/constants.hpp"
#include "util/join.hpp"
#include "util/json.hpp"
#include "util/span.hpp"

using nlohmann::json;

namespace cli {
    
namespace {
    namespace detail {
        template <typename T>
        json make_request(std::string_view cmd, std::optional<T> arg)
        {
            json req;
            req["type"] = "scheduler";
            req["cmd"] = std::string{cmd};

            if (arg)
            {
                req["arg"] = arg.value();
            }

            return req;
        }

        // Attempt to parse argument as a T. If an argument is given but cannot be parsed,
        // we throw. If no argument is given, we only want to fetch the current time limit.
        template <typename T>
        T parse_number(std::string_view arg)
        {
            T parse_result{};
            const auto [ptr, err] =
                std::from_chars(arg.data(), arg.data() + arg.size(), parse_result);

            if ((err != std::errc{}) || (ptr != arg.data() + arg.size()))
            {
                throw std::invalid_argument{std::string{arg} + " could not be parsed to a number"};
            }

            return parse_result;
        }
    }  // namespace detail

    void print_help()
    {
        // clang-format off
        static const std::string_view HELP_TEXT =
            "Available scheduler commands: spannersctl scheduler { time-limit | process-limit | resource-limit | sleep } [value]\n"
            "If the optional argument is omitted, the current value of the selected option is fetched.\n"
            "If the optional argument is given, the argument will be set as the new value.";
        // clang-format on

        std::cout << HELP_TEXT << std::endl;
    }

    exit_code time_limit(span<std::string_view> args)
    {
        std::optional<int64_t> limit;

        if (!args.empty())
        {
            try
            {
                limit = detail::parse_number<int64_t>(args.front());
            }
            catch (std::invalid_argument const &ex)
            {
                std::cerr << ex.what() << std::endl;
                return exit_code::ERROR;
            }
        }

        auto req = detail::make_request("time-limit", limit);
        io::instance().send(std::move(req));
        const auto msg = io::instance().receive();

        if (msg.at("status") != "ok")
        {
            std::cerr << "A server error occurred:\n";
            util::print(std::cerr, msg.at("error"));
            return exit_code::ERROR;
        }

        if (args.empty())
        {
            // This was a "get"-type request, so we should have gotten an answer.
            util::print(std::cout, msg.at("message"));
        }

        return exit_code::OK;
    }

    exit_code process_limit(span<std::string_view> args)
    {
        std::optional<size_t> limit;

        if (!args.empty())
        {
            try
            {
                limit = detail::parse_number<int64_t>(args.front());
            }
            catch (std::invalid_argument const &ex)
            {
                std::cerr << ex.what() << std::endl;
                return exit_code::ERROR;
            }
        }

        auto req = detail::make_request("process-limit", limit);
        io::instance().send(std::move(req));
        const auto msg = io::instance().receive();

        if (msg.at("status") != "ok")
        {
            std::cerr << "A server error occurred:\n";
            util::print(std::cerr, msg.at("error"));
            return exit_code::ERROR;
        }

        if (args.empty())
        {
            // This was a "get"-type request, so we should have gotten an answer.
            util::print(std::cout, msg.at("message"));
        }

        return exit_code::OK;
    }

    exit_code resource_limit(span<std::string_view> args)
    {
        std::optional<rlim64_t> limit;

        if (!args.empty())
        {
            try
            {
                limit = detail::parse_number<int64_t>(args.front());
            }
            catch (std::invalid_argument const &ex)
            {
                std::cerr << ex.what() << std::endl;
                return exit_code::ERROR;
            }
        }

        auto req = detail::make_request("resource-limit", limit);
        io::instance().send(std::move(req));
        const auto msg = io::instance().receive();

        if (msg.at("status") != "ok")
        {
            std::cerr << "A server error occurred:\n";
            util::print(std::cerr, msg.at("error"));
            return exit_code::ERROR;
        }

        if (args.empty())
        {
            // This was a "get"-type request, so we should have gotten an answer.
            util::print(std::cout, msg.at("message"));
        }

        return exit_code::OK;
    }

    exit_code sleep(span<std::string_view> args)
    {
        std::optional<int64_t> dur;

        if (!args.empty())
        {
            try
            {
                dur = detail::parse_number<int64_t>(args.front());
            }
            catch (std::invalid_argument const &ex)
            {
                std::cerr << ex.what() << std::endl;
                return exit_code::ERROR;
            }
        }

        auto req = detail::make_request("sleep", dur);
        io::instance().send(std::move(req));
        const auto msg = io::instance().receive();

        if (msg.at("status") != "ok")
        {
            std::cerr << "A server error occurred:\n";
            util::print(std::cerr, msg.at("error"));
            return exit_code::ERROR;
        }

        if (args.empty())
        {
            // This was a "get"-type request, so we should have gotten an answer.
            util::print(std::cout, msg.at("message"));
        }

        return exit_code::OK;
    }
}  // namespace

namespace scheduler {
    exit_code handle(span<std::string_view> args)
    {
        if (args.empty())
        {
            print_help();
            return exit_code::OK;
        }

        const auto &sc = args.front();
        exit_code ec;
        try
        {
            if (sc == "time-limit")
            {
                ec = time_limit(args.tail());
            }
            else if (sc == "process-limit")
            {
                ec = process_limit(args.tail());
            }
            else if (sc == "resource-limit")
            {
                ec = resource_limit(args.tail());
            }
            else if (sc == "sleep")
            {
                ec = sleep(args.tail());
            }
            else
            {
                print_help();
                return exit_code::ERROR;
            }
        }
        catch (json::exception &error)
        {
            std::cerr << "Server sent invalid data" << std::endl;
            return exit_code::ERROR;
        }

        return ec;
    }
}  // namespace scheduler

}  // namespace cli
