#include "networking/responses/origin_graph_response.hpp"

namespace server {

server::origin_graph_response::origin_graph_response(
    graphs::OriginGraphResponse &&origin_graph_response, status_code status)
    : abstract_response{response_type::ORIGIN_GRAPH, status}
    , m_origin_graph_response{std::move(origin_graph_response)}
{
}

graphs::OriginGraphResponse server::origin_graph_response::take_origin_graph_response()
{
    return std::move(m_origin_graph_response);
}

}  // namespace server
