#pragma once

#include <memory>

#include "container.pb.h"

namespace server {

class abstract_response;  // forward declaration
namespace response_factory {
    graphs::ResponseContainer build_response(std::unique_ptr<abstract_response> response);
}  // namespace response_factory

}  // namespace server
