#pragma once

#include <string>

#include <nlohmann/json.hpp>

namespace cli {
namespace util {
    void print(std::ostream &os, nlohmann::json const &jv);
}  // namespace util
}  // namespace cli
