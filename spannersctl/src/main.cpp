#include <iostream>

#include "subcommands/constants.hpp"
#include "subcommands/job.hpp"
#include "subcommands/scheduler.hpp"
#include "subcommands/user.hpp"
#include "util/span.hpp"

void print_help()
{
    // clang-format off
    static const std::string_view HELP_TEXT =
        "Usage: spannersctl { job | user | scheduler } ...\n"
        "Use any of the subcommands to get further help about a specific subcommand.";
    // clang-format on

    std::cout << HELP_TEXT << std::endl;
}

int main(int argc, const char **argv)
{
    if (argc == 1)
    {
        print_help();
        return static_cast<int>(cli::exit_code::OK);
    }

    std::vector<std::string_view> _{argv + 1, argv + argc};  // Purposefully omit executable name
    cli::span<std::string_view> args{_};

    cli::exit_code ec;
    const auto &command = args.front();
    if (command == "job")
    {
        ec = cli::job::handle(args.tail());
    }
    else if (command == "scheduler")
    {
        ec = cli::scheduler::handle(args.tail());
    }
    else if (command == "user")
    {
        ec = cli::user::handle(args.tail());
    }
    else
    {
        print_help();
        return static_cast<int>(cli::exit_code::ERROR);
    }

    return static_cast<int>(ec);
}
