#pragma once

#include "container.pb.h"

namespace server {

enum class response_type {
    UNDEFINED_RESPONSE = graphs::RequestType::UNDEFINED_REQUEST,
    SHORTEST_PATH = graphs::RequestType::SHORTEST_PATH,
    GENERIC = graphs::RequestType::GENERIC
};

}  // namespace server
