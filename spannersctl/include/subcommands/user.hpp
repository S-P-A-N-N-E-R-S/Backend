#pragma once

#include "subcommands/constants.hpp"
#include "util/span.hpp"

namespace cli {
namespace user {
    exit_code handle(span<std::string_view> args);
}  // namespace user
}  // namespace cli
