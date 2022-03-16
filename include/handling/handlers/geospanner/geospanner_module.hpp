#include "handling/handler_utilities.hpp"
#include "handling/handlers/geospanner/handler_delta_greedy_euclid.hpp"
#include "handling/handlers/geospanner/handler_path_greedy_euclid.hpp"
#include "handling/handlers/geospanner/handler_yao_graph_euclid.hpp"
#include "handling/handlers/geospanner/handler_yao_parametrized_euclid.hpp"
#include "handling/handlers/geospanner/handler_yao_pruning_euclid.hpp"

namespace server {
/**
 * @brief Shorthand command to register all geospanner modules at once
 * 
 */
inline void register_geospanner()
{
    handler_utilities::register_handler<delta_greedy_euclid_handler>("Geospanner");
    handler_utilities::register_handler<path_greedy_euclid_handler>("Geospanner");
    handler_utilities::register_handler<yao_graph_euclid_handler>("Geospanner");
    handler_utilities::register_handler<yao_parametrized_euclid_handler>("Geospanner");
    handler_utilities::register_handler<yao_pruning_euclid_handler>("Geospanner");
}

}  // namespace server