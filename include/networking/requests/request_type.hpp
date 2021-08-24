#pragma once

#include "container.pb.h"

namespace server {

enum class request_type {
    UNDEFINED_REQUEST = graphs::RequestType::UNDEFINED_REQUEST,
    SHORTEST_PATH = graphs::RequestType::SHORTEST_PATH,
    GENERIC = graphs::RequestType::GENERIC,
    AVAILABLE_HANDLERS = graphs::RequestType::AVAILABLE_HANDLERS
};

}  // namespace server
