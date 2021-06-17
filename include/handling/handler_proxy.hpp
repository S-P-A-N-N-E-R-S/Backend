#ifndef BASE_HANDLER_H_
#define BASE_HANDLER_H_

#include <string>

// OGDF include
#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

//handler includes
#include <handling/handlers/abstract_handler.hpp>

#include <networking/messages/graph_message.hpp>
#include <networking/requests/abstract_request.hpp>
#include <networking/requests/shortest_path_request.hpp>

namespace server {

class HandlerProxy
{
public:
    std::unique_ptr<abstract_response> handle(std::unique_ptr<abstract_request> request);
};

}  // namespace server
#endif  // BASE_HANDLER_H_
