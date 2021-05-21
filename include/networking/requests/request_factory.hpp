#pragma once

#include <memory>

#include "networking/requests/abstract_request.hpp"

#include "GraphData.pb.h"

namespace server {

class request_factory
{
public:
    request_factory() = default;
    ~request_factory() = default;

    std::unique_ptr<abstract_request> build_request(const graphs::RequestContainer &container);
};

}  // namespace server
