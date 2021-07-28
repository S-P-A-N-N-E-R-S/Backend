#include "networking/requests/request_factory.hpp"

#include "networking/requests/shortest_path_request.hpp"

namespace server {

std::unique_ptr<abstract_request> request_factory::build_request(
    const graphs::RequestContainer &container)
{
    switch (container.type())
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
        default: {
            // Unknown type
            return std::unique_ptr<abstract_request>(nullptr);
        }
        break;
    }
}

}  // namespace server
