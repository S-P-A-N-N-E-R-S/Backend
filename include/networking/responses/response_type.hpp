#pragma once

#include "container.pb.h"

namespace server {

enum class response_type {
    UNDEFINED_RESPONSE = graphs::ResponseContainer_ResponseType_UNDEFINED_RESPONSE,
    SHORTEST_PATH = graphs::ResponseContainer_ResponseType_SHORTEST_PATH
};

}  // namespace server
