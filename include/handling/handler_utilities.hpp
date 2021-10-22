#pragma once

#include <handling/handler_factory.hpp>
#include <networking/messages/meta_data.hpp>
#include <networking/responses/abstract_response.hpp>

namespace server {

/**
 * @brief Handle the given request by dynamically finding the correct registered handler.
 *
 * @param requestData (uncompressed) data from persistence
 * @return response container as well as the OGDF time
 */
std::pair<graphs::ResponseContainer, long> handle(const meta_data &meta,
                                                  graphs::RequestContainer &requestData);

/**
 * @brief 
 * 
 * @return std::unique_ptr<abstract_response> 
 */
std::unique_ptr<abstract_response> available_handlers();

namespace handler_utilities {

    using factory_map =
        std::unordered_map<std::string, std::unique_ptr<const abstract_handler_factory>>;

    /**
     * @brief Allows access to the factories map. This allows access to the handlers by their respective keys
     * 
     * @return factory_map& 
     */
    factory_map &handler_factories();

    /**
     * @brief Shortcut to register a new handler
     * 
     * @tparam handler_class the handler class to register
     * @param key the key in the factoories map
     */
    template <class handler_class>
    void register_handler(const std::string &key)
    {
        if (handler_factories().find(key) != handler_factories().end())
        {
            throw std::runtime_error("Second registration of handler with key " + key +
                                     " attempted!");
        }
        handler_factories()[key] = std::make_unique<const handler_factory<handler_class>>();
    }

    /**
     * @brief Utility function to initialize handlers. First call to it initializes the handler factories,
     * all following calls have no effect. 
     * User does not need to call it, it gets called automatically on startup.
     */
    void init_handlers();
}  // namespace handler_utilities

}  // namespace server
