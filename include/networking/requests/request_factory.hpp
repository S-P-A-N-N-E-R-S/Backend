#pragma once

#include <memory>

#include "container.pb.h"
#include "meta.pb.h"

namespace server {

class abstract_request;  // forward declaration
namespace request_factory {
    std::unique_ptr<abstract_request> build_request(graphs::RequestType type,
                                                    const graphs::RequestContainer &container);
}  // namespace request_factory

}  // namespace server
