#include <handling/handler_utilities.hpp>

// #####   default handler includes   #####

#include <handling/handlers/dijkstra_handler.hpp>
#include <handling/handlers/greedy_spanner_handler.hpp>

#include <ogdf/graphalg/SpannerBasicGreedy.h>
#include <ogdf/graphalg/SpannerBerman.h>
#include <handling/handlers/general_spanner_handler.hpp>

// #####   add your own handler includes below   #####

// #####   end of handler includes   #####

namespace server::handler_utilities {

void init_handlers()
{
    static bool initialized = false;
    if (initialized)
    {
        return;
    }
    initialized = true;

    // ##### default handler registers   #####

    register_handler<dijkstra_handler>();
    register_handler<greedy_spanner_handler>();

    register_handler<general_spanner_handler<ogdf::SpannerBerman<double>>>("spanner");
    register_handler<general_spanner_handler<ogdf::SpannerBasicGreedy<double>>>("spanner");

    // #####   register your own handlers below  #####

    // #####   end of handler registering   #####
}
}  // namespace server::handler_utilities