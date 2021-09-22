#pragma once

#include <networking/messages/meta_data.hpp>
#include <networking/responses/abstract_response.hpp>

namespace server {

class handler_proxy
{
public:
    /**
     * @brief Handle the given request by dynamically finding the correct registered handler.
     *
     * @param requestData (uncompressed) data from persistence
     * @return response container as well as the OGDF time
     */
    std::pair<graphs::ResponseContainer, long> handle(const meta_data &meta,
                                                      graphs::RequestContainer &requestData);

    std::unique_ptr<abstract_response> available_handlers();
};

}  // namespace server
