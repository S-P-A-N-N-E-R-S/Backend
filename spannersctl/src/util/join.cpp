#include "util/join.hpp"

namespace cli {
namespace util {
    std::string join(span<std::string_view> parts, std::string_view joiner)
    {
        // Allocate a std::string with the right capacity
        const auto sum_parts = [parts] {
            size_t sum{0};
            for (const auto p : parts)
            {
                sum += p.size();
            }

            return sum;
        }();

        std::string result;
        result.reserve(sum_parts + (parts.size() - 1) * joiner.size());

        // Actually build the std::string
        for (const auto p : parts.first(parts.size() - 1))
        {
            result += p;
            result += joiner;
        }

        result += parts.back();

        return result;
    }
}  // namespace util
}  // namespace cli
