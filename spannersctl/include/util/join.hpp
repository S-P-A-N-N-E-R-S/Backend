#pragma once

#include "util/span.hpp"

namespace cli {
namespace util {
    std::string join(span<std::string_view> parts, std::string_view joiner = " ");
}  // namespace util
}  // namespace cli
