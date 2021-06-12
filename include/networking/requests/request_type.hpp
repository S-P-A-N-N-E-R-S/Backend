#pragma once

#include "container.pb.h"

namespace server {

enum class request_type {
    UNDEFINED_REQUEST = graphs::RequestContainer_RequestType_UNDEFINED_REQUEST,
    SHORTEST_PATH = graphs::RequestContainer_RequestType_SHORTEST_PATH
};

}  // namespace server
