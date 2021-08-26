#pragma once

#include "meta.pb.h"

namespace server {

enum class response_type {
    UNDEFINED_RESPONSE = graphs::RequestType::UNDEFINED_REQUEST,
    SHORTEST_PATH = graphs::RequestType::SHORTEST_PATH,
    GENERIC = graphs::RequestType::GENERIC,
    AVAILABLE_HANDLERS = graphs::RequestType::AVAILABLE_HANDLERS,
    NEW_JOB = graphs::RequestType::NEW_JOB_RESPONSE,
    STATUS = graphs::RequestType::STATUS
};

}  // namespace server
