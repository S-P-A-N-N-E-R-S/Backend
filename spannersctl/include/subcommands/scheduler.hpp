#pragma once

#include "subcommands/constants.hpp"
#include "util/span.hpp"

namespace cli {
namespace scheduler {
    exit_code handle(span<std::string_view> args);
}  // namespace scheduler
}  // namespace cli
