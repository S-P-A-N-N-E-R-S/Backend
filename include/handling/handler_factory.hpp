#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <networking/requests/abstract_request.hpp>
#include "available_handlers.pb.h"

namespace server {

// Forward declaration
class abstract_handler;

class abstract_handler_factory
{
public:
    virtual std::unique_ptr<abstract_handler> produce(
        std::unique_ptr<abstract_request> request) const = 0;

    virtual graphs::HandlerInformation handler_information() const = 0;
};

using factory_map = std::unordered_map<std::string, const abstract_handler_factory *>;

/**
 * @brief Allows access to the factories map. This allows access to the handlers by their respective keys
 * 
 * @return factory_map& 
 */
factory_map &handler_factories();

template <typename HandlerDerived>
class handler_factory : public abstract_handler_factory
{
    static_assert(std::is_base_of<abstract_handler, HandlerDerived>::value,
                  "HandlerDerived must derive from abstract_handler");

public:
    handler_factory(const std::string &key)
    {
        if (handler_factories().find(key) != handler_factories().end())
        {
            throw std::runtime_error("Second registration of handler with key " + key +
                                     " attempted!");
        }
        handler_factories()[key] = this;
    }

    /**
     * @brief Produces a handler of type E and hands ownership over to caller
     * 
     * @param request 
     * @return std::unique_ptr<abstract_handler> 
     */
    virtual std::unique_ptr<abstract_handler> produce(
        std::unique_ptr<abstract_request> request) const override
    {
        return std::make_unique<HandlerDerived>(std::move(request));
    }

    /**
     * @brief Get the Handler Information object
     * 
     * @return graphs::HandlerInformation* 
     */
    virtual graphs::HandlerInformation handler_information() const override
    {
        return HandlerDerived::handler_information();
    }
};

/**
 * @brief Register commando for handlers.
 */
#define register_handler(key, handler_class)                                                     \
    namespace {                                                                                  \
        const server::handler_factory<class server::handler_class> handler_class##_factory(key); \
    }

}  // namespace server
