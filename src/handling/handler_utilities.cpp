#include <handling/handler_utilities.hpp>
#include <handling/handlers/dijkstra_handler.hpp>
#include <iostream>
#include <networking/exceptions.hpp>
#include <networking/requests/request_factory.hpp>
#include <networking/responses/available_handlers_response.hpp>
#include <networking/responses/generic_response.hpp>

namespace server {

void copy_static_attributes(const graphs::RequestContainer &request_container,
                            generic_response *response)
{
    graphs::GenericRequest proto_request;
    request_container.request().UnpackTo(&proto_request);

    response->m_static_attributes = proto_request.staticattributes();
}

handle_return handle(const meta_data &meta, graphs::RequestContainer &requestData)
{
    auto request = request_factory::build_request(meta.request_type, requestData);
    handle_return response;
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

            if (response.response_abstract->type() == response_type::GENERIC)
            {
                auto response_generic =
                    static_cast<generic_response *>(response.response_abstract.get());

                // We need to manually copy over static attributes because we cannot rely on the handler
                // doing so
                copy_static_attributes(requestData, response_generic);
                response.response_proto =
                    response_factory::build_response(std::move(response.response_abstract));
                response.response_abstract = nullptr;
            }

            break;
        }
        default: {
            throw request_parse_error("No matching request type!", request_type::UNDEFINED_REQUEST);
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
