#include "networking/requests/request_factory.hpp"

#include "networking/requests/generic_request.hpp"
#include "networking/requests/shortest_path_request.hpp"

namespace server {

std::unique_ptr<abstract_request> request_factory::build_request(
    graphs::RequestType type, const graphs::RequestContainer &container)
{
    switch (type)
    {
        case graphs::RequestType::SHORTEST_PATH: {
            // We know that the container contains a shortest path request
            graphs::ShortestPathRequest proto_request;
            const bool ok = container.request().UnpackTo(&proto_request);

            if (!ok)
            {
                return std::unique_ptr<abstract_request>(nullptr);
            }

            auto retval = std::make_unique<shortest_path_request>(proto_request);
            return retval;
        }
        break;
        case graphs::RequestType::GENERIC: {
            graphs::GenericRequest proto_request;

            if (const bool ok = container.request().UnpackTo(&proto_request); !ok)
            {
                return std::unique_ptr<abstract_request>(nullptr);
            }

            auto retval = std::make_unique<generic_request>(proto_request);
            return retval;
        }
        break;
        default: {
            // Unknown type
            return std::unique_ptr<abstract_request>(nullptr);
        }
        break;
    }
}

}  // namespace server
