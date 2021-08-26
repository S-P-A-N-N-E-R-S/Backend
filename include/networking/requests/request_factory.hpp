#pragma once

#include <memory>

#include "networking/requests/abstract_request.hpp"

#include "container.pb.h"
#include "meta.pb.h"

namespace server {

class request_factory
{
public:
    request_factory() = default;
    ~request_factory() = default;

    std::unique_ptr<abstract_request> build_request(graphs::RequestType type,
                                                    const graphs::RequestContainer &container);
};

}  // namespace server
