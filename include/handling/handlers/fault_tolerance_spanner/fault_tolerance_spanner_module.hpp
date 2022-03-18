#include "handling/handler_utilities.hpp"
#include "handling/handlers/fault_tolerance_spanner/handler_bodwin_dinitz.hpp"
#include "handling/handlers/fault_tolerance_spanner/handler_chechik_langberg_edges.hpp"
#include "handling/handlers/fault_tolerance_spanner/handler_chechik_langberg_vertices.hpp"
#include "handling/handlers/fault_tolerance_spanner/handler_dinitz_krauthgamer.hpp"
#include "handling/handlers/fault_tolerance_spanner/handler_resilient.hpp"

namespace server {
/**
 * @brief Shorthand command to register all fault tolerance spanner modules at once
 *
 */
inline void register_fault_tolerance_spanner()
{
    handler_utilities::register_handler<bodwin_dinitz_handler>("Fault Tolerance Spanner");
    handler_utilities::register_handler<dinitz_krauthgamer_handler>("Fault Tolerance Spanner");
    handler_utilities::register_handler<chechik_langberg_edges_handler>("Fault Tolerance Spanner");
    handler_utilities::register_handler<chechik_langberg_vertices_handler>(
        "Fault Tolerance Spanner");
    handler_utilities::register_handler<resilient_handler>("Fault Tolerance Spanner");
}

}  // namespace server
