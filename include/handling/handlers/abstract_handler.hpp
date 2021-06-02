#ifndef HANDLERS_HANDLER_H_
#define HANDLERS_HANDLER_H_

#include <GraphData.pb.h>
#include <string>
#include <vector>

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
};

}  // namespace server
#endif  // HANDLERS_HANDLER_H_