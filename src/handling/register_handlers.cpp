#include <handling/handler_factory.hpp>

// #####   default handler includes   #####

#include <handling/handlers/dijkstra_handler.hpp>

#include <handling/handlers/greedy_spanner_handler.hpp>

#include <ogdf/graphalg/SpannerBerman.h>
#include <handling/handlers/general_spanner_handler.hpp>

// #####   add your own handler includes below   #####

// #####   end of handler includes   #####

namespace server {

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
        throw std::runtime_error("Second registration of handler with key " + key + " attempted!");
    }
    handler_factories()[key] = std::make_unique<const handler_factory<handler_class>>();
}

void init_handlers()
{
    static bool initialized = false;
    if (initialized)
    {
        return;
    }
    initialized = true;

    // ##### default handler registers   #####

    register_handler<dijkstra_handler>("dijkstra");

    register_handler<greedy_spanner_handler>("greedy_spanner");

    register_handler<general_spanner_handler<ogdf::SpannerBerman<double>>>(
        general_spanner_handler<ogdf::SpannerBerman<double>>::name());

    // #####   register your own handlers below  #####

    // #####   end of handler registering   #####
}
}  //  namespace server