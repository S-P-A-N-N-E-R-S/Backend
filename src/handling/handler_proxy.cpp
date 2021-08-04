#include <handling/handler_proxy.hpp>
#include <handling/handlers/dijkstra_handler.hpp>
#include <iostream>
#include <networking/requests/request_factory.hpp>

namespace server {

/**
 * @brief Old method used in prototype
 * 
 * @param request 
 * @return std::unique_ptr<abstract_response> 
 */
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

/**
 * @brief New WIP variant using persistence_mock data
 * 
 * @param requestData simulates (uncompressed) data from persistence
 * @return std::unique_ptr<abstract_response> 
 */
std::pair<graphs::ResponseContainer, long> HandlerProxy::handle(
    graphs::RequestContainer &requestData)
{
    server::request_factory factory;
    std::unique_ptr<server::abstract_request> request = factory.build_request(requestData);
    std::pair<graphs::ResponseContainer, long> response;
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

            DijkstraHandler h(*sp_request);
            response = h.handle_new();
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
