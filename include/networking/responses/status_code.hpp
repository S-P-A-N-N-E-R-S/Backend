#pragma once

#include "GraphData.pb.h"

namespace server {

enum status_code {
    OK = graphs::ResponseContainer_StatusCode_OK,
    ERROR = graphs::ResponseContainer_StatusCode_ERROR
};

}  // namespace server
