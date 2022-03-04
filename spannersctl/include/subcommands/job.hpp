#pragma once

#include "subcommands/constants.hpp"
#include "util/span.hpp"

namespace cli {
namespace job {
    exit_code handle(span<std::string_view> args);
}  // namespace job
}  // namespace cli
