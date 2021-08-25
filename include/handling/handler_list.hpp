#pragma once

// Warning: Including this file more than once WILL fail your build process.
// By including it, the contained handlers are registered and compiled.

#include <handling/handlers/dijkstra_handler.hpp>
register_handler("dijkstra", dijkstra_handler)
