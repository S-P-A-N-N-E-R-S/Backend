#include <handling/handler_list.hpp>
#include <handling/handler_proxy.hpp>
#include <handling/handlers/dijkstra_handler.hpp>
#include <iostream>
#include <networking/requests/request_factory.hpp>
#include <networking/responses/available_handlers_response.hpp>

namespace server {

std::pair<graphs::ResponseContainer, long> handler_proxy::handle(
    const meta_data &meta, graphs::RequestContainer &requestData)
{
    server::request_factory factory;
    std::unique_ptr<server::abstract_request> request =
        factory.build_request(meta.request_type, requestData);
    std::pair<graphs::ResponseContainer, long> response;
    switch (request->type())
    {
        case request_type::GENERIC: {
            const auto *handler_name_finder = dynamic_cast<generic_request *>(request.get());
            if (!handler_name_finder)
            {
                throw std::runtime_error("handler_proxy: dynamic_cast failed!");
            }

            auto &factories = handler_factories();

            auto factory = factories.at(meta.handler_type);

            auto handler = factory->produce(std::move(request));

            response = handler->handle();

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

std::unique_ptr<abstract_response> handler_proxy::available_handlers()
{
    auto handler_information = std::make_unique<graphs::AvailableHandlersResponse>();
    auto handlers = handler_information->mutable_handlers();

    for (auto &it : handler_factories())
    {
        handlers->Add(it.second->handler_information());
    }

    return std::make_unique<available_handlers_response>(std::move(handler_information),
                                                         status_code::OK);
}

}  //namespace server
