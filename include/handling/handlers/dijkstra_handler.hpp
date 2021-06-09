#ifndef HANDLERS_DIJKSTRA_HANDLER_H_
#define HANDLERS_DIJKSTRA_HANDLER_H_

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/graphalg/Dijkstra.h>
#include <handling/handlers/abstract_handler.hpp>

#include <networking/requests/shortest_path_request.hpp>
#include <networking/responses/shortest_path_response.hpp>

namespace server {

class DijkstraHandler : AbstractHandler
{
public:
    DijkstraHandler(shortest_path_request &sp_request);

    virtual std::unique_ptr<abstract_response> handle();

private:
    const graph_message *m_req_message;
    bool m_directed;
    ogdf::node m_start;
    const ogdf::Graph &m_graph;
    ogdf::EdgeArray<double> m_weights;
};

}  // namespace server
#endif  // HANDLERS_DIJKSTRA_HANDLER_H_