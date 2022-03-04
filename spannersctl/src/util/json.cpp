#include "util/json.hpp"

#include <string>

namespace cli {
namespace util {
    void print(std::ostream &os, nlohmann::json const &jv)
    {
        os << jv.dump(4) << '\n';
    }
}
}  // namespace cli
