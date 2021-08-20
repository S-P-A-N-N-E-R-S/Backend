#pragma once

#include <networking/responses/abstract_response.hpp>

namespace server {

// Forward Declaration
struct resultObject;

/**
 * @brief Class template for handler classes. Not much at the moment
 * 
 */
class AbstractHandler
{
public:
    virtual std::unique_ptr<abstract_response> handle() = 0;
    virtual std::pair<std::unique_ptr<graphs::ResponseContainer>, long> handle_new() = 0;
};

}  // namespace server
