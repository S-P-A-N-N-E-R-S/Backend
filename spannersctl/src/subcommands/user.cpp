#include "subcommands/user.hpp"

#include <iostream>

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
        // TODO: Might have to add a limit here?
        json fetch_users()
        {
            auto req = [] {
                json req;
                req["type"] = "user";
                req["cmd"] = "list";

                return req;
            }();

            io::instance().send(std::move(req));
            auto resp = io::instance().receive();

            return resp;
        }

        json fetch_user_info(std::string_view name_or_id)
        {
            auto req = [name_or_id] {
                json req;
                req["type"] = "user";
                req["cmd"] = "info";
                req["arg"] = std::string{name_or_id};

                return req;
            }();

            io::instance().send(std::move(req));
            auto resp = io::instance().receive();

            return resp;
        }

        json delete_user(std::string_view name_or_id)
        {
            auto req = [name_or_id] {
                json req;
                req["type"] = "user";
                req["cmd"] = "delete";
                req["arg"] = std::string{name_or_id};

                return req;
            }();

            io::instance().send(std::move(req));
            auto resp = io::instance().receive();

            return resp;
        }

        json block_user(std::string_view name_or_id)
        {
            auto req = [name_or_id] {
                json req;
                req["type"] = "user";
                req["cmd"] = "block";
                req["arg"] = std::string{name_or_id};

                return req;
            }();

            io::instance().send(std::move(req));
            auto resp = io::instance().receive();

            return resp;
        }

        json unblock_user(std::string_view name_or_id)
        {
            auto req = [name_or_id] {
                json req;
                req["type"] = "user";
                req["cmd"] = "unblock";
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
            "Available user commands: spannersctl user { block <user> | delete <user> | list | info <user> }\n"
            "Users can be identified by their name or ID.\n"
            "    block <user>   -- block the user from submitting any further requests.\n"
            "    unblock <user> -- unblock the user from submitting any further requests.\n"
            "    delete <user>  -- deletes the user and all associated jobs.\n"
            "    list           -- list all users.\n"
            "    info <user>    -- print detailed information about a single user.";
        // clang-format on

        std::cout << HELP_TEXT << std::endl;
    }

    exit_code list(span<std::string_view> /*args*/)  // TODO: Do we even need args?
    {
        const auto msg = detail::fetch_users();

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
        const auto msg = detail::fetch_user_info(name_or_id);

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
        const auto msg = detail::delete_user(name_or_id);

        if (msg.at("status") != "ok")
        {
            std::cerr << "A server error occurred:\n";
            util::print(std::cerr, msg.at("error"));
            return exit_code::ERROR;
        }

        return exit_code::OK;
    }

    exit_code block(span<std::string_view> args)
    {
        if (args.empty())
        {
            print_help();
            return exit_code::ERROR;
        }

        const auto name_or_id = util::join(args);
        const auto msg = detail::block_user(name_or_id);

        if (msg.at("status") != "ok")
        {
            std::cerr << "A server error occurred:\n";
            util::print(std::cerr, msg.at("error"));
            return exit_code::ERROR;
        }

        return exit_code::OK;
    }

    exit_code unblock(span<std::string_view> args)
    {
        if (args.empty())
        {
            print_help();
            return exit_code::ERROR;
        }

        const auto name_or_id = util::join(args);
        const auto msg = detail::unblock_user(name_or_id);

        if (msg.at("status") != "ok")
        {
            std::cerr << "A server error occurred:\n";
            util::print(std::cerr, msg.at("error"));
            return exit_code::ERROR;
        }

        return exit_code::OK;
    }
}  // namespace

namespace user {
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
            else if (sc == "block")
            {
                ec = block(args.tail());
            }
            else if (sc == "unblock")
            {
                ec = unblock(args.tail());
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
}  // namespace user

}  // namespace cli
