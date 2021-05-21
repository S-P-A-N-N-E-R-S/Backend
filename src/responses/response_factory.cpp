#include "networking/responses/response_factory.hpp"
#include "networking/responses/shortest_path_response.hpp"

#include "GraphData.pb.h"

namespace server {

namespace {
    graphs::ResponseContainer_StatusCode to_proto_status(status_code code)
    {
        switch (code)
        {
            case status_code::OK: {
                return graphs::ResponseContainer_StatusCode::ResponseContainer_StatusCode_OK;
            }
            break;
            case status_code::ERROR: {
                return graphs::ResponseContainer_StatusCode::ResponseContainer_StatusCode_ERROR;
            }
            break;
            default: {
                // We should never get here
                assert(false);
                // TODO: Replace with UNDEFINED once defined in protobuf
                return graphs::ResponseContainer_StatusCode::ResponseContainer_StatusCode_ERROR;
            }
            break;
        }
    }

    graphs::ResponseContainer_ResponseType to_proto_type(response_type type)
    {
        switch (type)
        {
            case response_type::SHORTEST_PATH: {
                return graphs::ResponseContainer_ResponseType::
                    ResponseContainer_ResponseType_SHORTEST_PATH;
            }
            break;
            default: {
                // We should never get here
                assert(false);
                // TODO: Replace with UNDEFINED once defined in protobuf
                return graphs::ResponseContainer_ResponseType::
                    ResponseContainer_ResponseType_SHORTEST_PATH;
            }
            break;
        }
    }
}

std::unique_ptr<graphs::ResponseContainer> response_factory::build_response(
    std::unique_ptr<abstract_response> &response)
{
    auto proto_container = std::make_unique<graphs::ResponseContainer>();

    proto_container->set_status(to_proto_status(response->status()));
    proto_container->set_type(to_proto_type(response->type()));

    if (response->status() != status_code::OK)
    {
        return proto_container;
    }

    switch (response->type())
    {
        case response_type::SHORTEST_PATH: {
            // We know that we got a shortest path response
            auto *spr = static_cast<shortest_path_response *>(response.get());

            graphs::ShortPathResponse proto_response;

            proto_response.set_allocated_graph(spr->take_proto_graph().release());
            const bool ok = proto_container->mutable_response()->PackFrom(proto_response);

            if (!ok)
            {
                proto_container->set_status(to_proto_status(status_code::ERROR));
                return proto_container;
            }
        }
        break;
        default: {
            // Unknown type
            proto_container->set_status(to_proto_status(status_code::ERROR));
            return proto_container;
        }
        break;
    }

    return proto_container;
}

}  // namespace server
