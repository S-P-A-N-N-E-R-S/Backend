#include <handling/handler_proxy.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <handling/handlers/dijkstra_handler.hpp>

namespace server {

std::unique_ptr<abstract_response> HandlerProxy::handle(std::unique_ptr<abstract_request> request)
{
    std::unique_ptr<abstract_response> response;
    switch (request->type())
    {
        case request_type::SHORTEST_PATH: {
            shortest_path_request *sp_request =
                dynamic_cast<shortest_path_request *>(request.get());

            if (sp_request == nullptr)
            {
                //ToDo: Lets crash the server because I'm to lazy for real error handling in the prototype
                throw std::runtime_error("handlerProxy: dynamic_cast failed!");
            }

            DijkstraHandler h = DijkstraHandler(*sp_request);

            response = h.handle();
            break;
        }
        default: {
            //ToDo: Real error handling
            std::cout << "No valid request Type" << std::endl;
            break;
        }
    }

    //ToDo: Store result in database and provide some kind of status update system
    return response;
}

}  //namespace server