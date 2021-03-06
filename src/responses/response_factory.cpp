#include "networking/responses/response_factory.hpp"

#include "networking/responses/available_handlers_response.hpp"
#include "networking/responses/generic_response.hpp"
#include "networking/responses/new_job_response.hpp"
#include "networking/responses/origin_graph_response.hpp"
#include "networking/responses/shortest_path_response.hpp"
#include "networking/responses/status_response.hpp"

#include "container.pb.h"

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
                return graphs::ResponseContainer_StatusCode::
                    ResponseContainer_StatusCode_UNDEFINED_STATUS;
            }
            break;
        }
    }

    graphs::RequestType to_proto_type(response_type type)
    {
        switch (type)
        {
            case response_type::AVAILABLE_HANDLERS: {
                return graphs::RequestType::AVAILABLE_HANDLERS;
            }
            break;
            case response_type::SHORTEST_PATH: {
                return graphs::RequestType::SHORTEST_PATH;
            }
            break;
            case response_type::GENERIC: {
                return graphs::RequestType::GENERIC;
            }
            break;
            default: {
                return graphs::RequestType::UNDEFINED_REQUEST;
            }
            break;
        }
    }
}  // namespace

namespace response_factory {
    graphs::ResponseContainer build_response(std::unique_ptr<abstract_response> response)
    {
        graphs::ResponseContainer proto_container;

        proto_container.set_status(to_proto_status(response->status()));

        if (response->status() != status_code::OK)
        {
            return proto_container;
        }

        switch (response->type())
        {
            case response_type::SHORTEST_PATH: {
                // We know that we got a shortest path response
                auto *spr = static_cast<shortest_path_response *>(response.get());

                graphs::ShortestPathResponse proto_response;

                proto_response.set_allocated_graph(spr->take_proto_graph().release());

                auto *coords = spr->take_vertex_coords().release();
                proto_response.mutable_vertexcoordinates()->Swap(coords);
                delete coords;  // This contains the empty "default value" map after the swap

                auto *costs = spr->take_edge_costs().release();
                proto_response.mutable_edgecosts()->Swap(costs);
                delete costs;

                const bool ok = proto_container.mutable_response()->PackFrom(proto_response);

                if (!ok)
                {
                    proto_container.set_status(to_proto_status(status_code::ERROR));
                    return proto_container;
                }
            }
            break;
            case response_type::GENERIC: {
                // We know that we got a generic response
                auto *gr = static_cast<generic_response *>(response.get());

                graphs::GenericResponse proto_response = gr->as_proto();
                const bool ok = proto_container.mutable_response()->PackFrom(proto_response);

                if (!ok)
                {
                    proto_container.set_status(to_proto_status(status_code::ERROR));
                }
            }
            break;
            case response_type::AVAILABLE_HANDLERS: {
                auto *ahr = static_cast<available_handlers_response *>(response.get());
                auto available_handlers = ahr->take_available_handlers();
                const bool ok = proto_container.mutable_response()->PackFrom(*available_handlers);
                if (!ok)
                {
                    proto_container.set_status(to_proto_status(status_code::ERROR));
                    return proto_container;
                }
            }
            break;
            case response_type::NEW_JOB: {
                auto *new_job_response = static_cast<server::new_job_response *>(response.get());
                auto new_job_response_graphs = new_job_response->take_new_job_response();
                const bool ok =
                    proto_container.mutable_response()->PackFrom(new_job_response_graphs);
                if (!ok)
                {
                    proto_container.set_status(to_proto_status(status_code::ERROR));
                    return proto_container;
                }
            }
            break;
            case response_type::ORIGIN_GRAPH: {
                auto *ogr = static_cast<server::origin_graph_response *>(response.get());
                auto origin_graph_response = ogr->take_origin_graph_response();
                const bool ok = proto_container.mutable_response()->PackFrom(origin_graph_response);
                if (!ok)
                {
                    proto_container.set_status(to_proto_status(status_code::ERROR));
                    return proto_container;
                }
            }
            break;
            case response_type::STATUS: {
                auto *stat_response = static_cast<status_response *>(response.get());
                auto stat_response_graphs = stat_response->take_status_response();
                const bool ok = proto_container.mutable_response()->PackFrom(stat_response_graphs);
                if (!ok)
                {
                    proto_container.set_status(to_proto_status(status_code::ERROR));
                    return proto_container;
                }
            }
            break;
            default: {
                // Unknown type
                proto_container.set_status(to_proto_status(status_code::ERROR));
                return proto_container;
            }
            break;
        }

        return proto_container;
    }
}  // namespace response_factory

}  // namespace server
