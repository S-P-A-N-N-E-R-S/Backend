#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>

#include <networking/requests/abstract_request.hpp>
#include "available_handlers.pb.h"

namespace server {

// Dummy struct to assert existence of handler_information method
template <class handler_class>
struct has_handler_information {
    template <typename signature, signature>
    struct equal_type_check;

    template <typename handler>
    static std::true_type internal_test_dummy(
        equal_type_check<graphs::HandlerInformation (*)(), &handler::handler_information> *);

    template <typename handler>
    static std::false_type internal_test_dummy(...);

    static const bool value = decltype(internal_test_dummy<handler_class>(nullptr))::value;
};

// Forward declaration
class abstract_handler;

class abstract_handler_factory
{
public:
    virtual std::unique_ptr<abstract_handler> produce(
        std::unique_ptr<abstract_request> request) const = 0;

    virtual graphs::HandlerInformation handler_information() const = 0;
};

using factory_map =
    std::unordered_map<std::string, std::unique_ptr<const abstract_handler_factory>>;

/**
 * @brief Allows access to the factories map. This allows access to the handlers by their respective keys
 * 
 * @return factory_map& 
 */
factory_map &handler_factories();

template <typename handler_derived>
class handler_factory : public abstract_handler_factory
{
    static_assert(std::is_base_of<abstract_handler, handler_derived>::value,
                  "handler_derived must derive from abstract_handler");

    static_assert(std::is_constructible<handler_derived, std::unique_ptr<abstract_request>>::value,
                  "handler_derived must provide a constructor with signature: "
                  "handler_derived(std::unique_ptr<abstract_request>)");

    static_assert(has_handler_information<handler_derived>::value,
                  "handler_derived must provide a static method with signature: "
                  "graphs::HandlerInformation handler_information()");

public:
    /**
     * @brief Produces a handler of type E and hands ownership over to caller
     * 
     * @param request 
     * @return std::unique_ptr<abstract_handler> 
     */
    virtual std::unique_ptr<abstract_handler> produce(
        std::unique_ptr<abstract_request> request) const override
    {
        return std::make_unique<handler_derived>(std::move(request));
    }

    /**
     * @brief Get the Handler Information object
     * 
     * @return graphs::HandlerInformation* 
     */
    virtual graphs::HandlerInformation handler_information() const override
    {
        return handler_derived::handler_information();
    }
};

/**
 * @brief Utility function to initialize handlers. First call to it initializes the handler factories,
 * all following calls have no effect. 
 * User does not need to call it, it gets called automatically on startup.
 */
void init_handlers();

}  // namespace server
