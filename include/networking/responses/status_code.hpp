#pragma once

#include "container.pb.h"

namespace server {

enum status_code {
    UNDEFINED_STATUS = graphs::ResponseContainer_StatusCode_UNDEFINED_STATUS,
    OK = graphs::ResponseContainer_StatusCode_OK,
    ERROR = graphs::ResponseContainer_StatusCode_ERROR
};

}  // namespace server
