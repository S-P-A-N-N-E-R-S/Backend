#pragma once

#include <string>
#include <vector>

#include <handling/handler_factory.hpp>
#include <networking/responses/abstract_response.hpp>
#include "available_handlers.pb.h"

namespace server {

/**
 * @brief Structure to store information which is returned from handlers and handler utilities
 * 
 */
struct handle_return {
    std::unique_ptr<abstract_response> response_abstract;
    long ogdf_time;
    graphs::ResponseContainer response_proto;
};

/**
 * @brief Class template for handler classes
 * 
 */
class abstract_handler
{
public:
    virtual handle_return handle() = 0;

    virtual ~abstract_handler() = default;

protected:
    /**
     * Shorthand function to create a HandlerInformation object with the given parameters set.
     */
    static graphs::HandlerInformation createHandlerInformation(const std::string &name,
                                                               graphs::RequestType type);

    /**
     * Adds the provided FieldInformation to information.
     * Choices must be added manually, there is not shorthand yet for this.
     * TODO: add default
     */
    static graphs::FieldInformation *addFieldInformation(graphs::HandlerInformation &information,
                                                         graphs::FieldInformation_FieldType type,
                                                         const std::string &label,
                                                         const std::string &key,
                                                         bool required = false);

    /**
     * Adds the provided ResultInformation to information.
     */
    static void addResultInformation(graphs::HandlerInformation &information,
                                     graphs::ResultInformation_HandlerReturnType type,
                                     const std::string &key, const std::string &label = "");
};

}  // namespace server
