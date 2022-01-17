#pragma once

#include <string>

#include <boost/json.hpp>

namespace cli {
namespace util {
    void print(std::ostream &os, boost::json::value const &jv, std::string *indent = nullptr);
}  // namespace util
}  // namespace cli
