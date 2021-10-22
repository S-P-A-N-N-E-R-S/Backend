#include <handling/handler_utilities.hpp>
#include <handling/handlers/dijkstra_handler.hpp>
#include <iostream>
#include <networking/requests/request_factory.hpp>
#include <networking/responses/available_handlers_response.hpp>

namespace server {

std::pair<graphs::ResponseContainer, long> handle(const meta_data &meta,
                                                  graphs::RequestContainer &requestData)
{
    auto request = request_factory::build_request(meta.request_type, requestData);
    std::pair<graphs::ResponseContainer, long> response;
    switch (request->type())
    {
        case request_type::GENERIC: {
            const auto *handler_name_finder = dynamic_cast<generic_request *>(request.get());
            if (!handler_name_finder)
            {
                throw std::runtime_error("handler_utilities: dynamic_cast failed!");
            }

            auto &factories = handler_utilities::handler_factories();

            const auto factory = factories.at(meta.handler_type).get();

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

    return response;
}

std::unique_ptr<abstract_response> available_handlers()
{
    auto handler_information = std::make_unique<graphs::AvailableHandlersResponse>();
    auto handlers = handler_information->mutable_handlers();

    for (auto &it : handler_utilities::handler_factories())
    {
        handlers->Add(it.second->handler_information());
    }

    return std::make_unique<available_handlers_response>(std::move(handler_information),
                                                         status_code::OK);
}

namespace handler_utilities {

    factory_map &handler_factories()
    {
        init_handlers();
        static factory_map factories = {};
        return factories;
    }

}  // namespace handler_utilities

}  //namespace server
