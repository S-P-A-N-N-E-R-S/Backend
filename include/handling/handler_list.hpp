// clang-format off
#pragma once

// Warning: Including this file more than once WILL fail your build process.
// By including it, the contained handlers are registered and compiled.

#include <handling/handlers/dijkstra_handler.hpp>
register_handler("dijkstra", dijkstra_handler)

#include <handling/handlers/greedy_spanner_handler.hpp>
register_handler("greedy_spanner", greedy_spanner_handler)
