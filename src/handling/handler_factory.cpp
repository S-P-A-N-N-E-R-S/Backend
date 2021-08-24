#include <handling/handler_factory.hpp>

namespace server {

factory_map &handler_factories()
{
    static factory_map factories = {};
    return factories;
}

}  // namespace server
