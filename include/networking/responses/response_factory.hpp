#pragma once

#include <memory>

#include "networking/responses/abstract_response.hpp"
#include "networking/responses/shortest_path_response.hpp"

#include "container.pb.h"

namespace server {

// TODO: Talk about potentially naming this differently
class response_factory
{
public:
    response_factory() = default;
    ~response_factory() = default;

    std::unique_ptr<graphs::ResponseContainer> build_response(
        std::unique_ptr<abstract_response> &response);
};

}  // namespace server
