#include "handling/handlers/mst_handler.hpp"

#include <chrono>
#include "networking/exceptions.hpp"
#include "networking/responses/generic_response.hpp"
#include "networking/responses/response_factory.hpp"

namespace server {

mst_handler::mst_handler(std::unique_ptr<abstract_request> request)
    : m_request{}
{
    if (const auto *type_check_ptr = dynamic_cast<generic_request *>(request.get());
        !type_check_ptr)
    {
        throw server::request_parse_error("mst_handler: dynamic_cast failed!", request->type(),
                                          "mst");
    }

    m_request = std::unique_ptr<generic_request>{static_cast<generic_request *>(request.release())};
}

std::pair<graphs::ResponseContainer, long> mst_handler::handle()
{
    const auto *graph_message = m_request->graph_message();
    auto &graph = graph_message->graph();

    const auto *og_edge_costs = m_request->edge_costs();

    auto start = std::chrono::high_resolution_clock::now();

	ogdf::EdgeArray<bool> in_mst(graph, false);
	computeMinST(graph, *og_edge_costs, in_mst);

    auto stop = std::chrono::high_resolution_clock::now();
    long ogdf_time = (std::chrono::duration_cast<std::chrono::microseconds>(stop - start)).count();

    // Build shortest path graph from origin to index
    auto mst = std::make_unique<ogdf::Graph>();
    auto mst_node_uids = std::make_unique<ogdf::NodeArray<uid_t>>(*mst);
    auto mst_edge_uids = std::make_unique<ogdf::EdgeArray<server::uid_t>>(*mst);
    auto mst_edge_costs = ogdf::EdgeArray<double>(*mst);
    auto mst_node_coords = ogdf::NodeArray<node_coordinates>(*mst);

    const auto &og_edge_uids = graph_message->edge_uids();
    const auto &og_node_uids = graph_message->node_uids();
    const auto *og_node_coords = m_request->node_coords();

    ogdf::NodeArray<ogdf::node> original_node_to_mst{graph};

    // add all nodes to answer (before adding edges)
    for (auto og_node : graph.nodes)
    {
        const auto mst_node = mst->newNode();
        original_node_to_mst[og_node] = mst_node;
        mst_node_uids->operator[](mst_node) = og_node_uids[og_node];
        mst_node_coords[mst_node] = og_node_coords->operator[](og_node);
    }

	for (ogdf::edge e : graph.edges) 
	{
		if (in_mst[e]) 
		{
			ogdf::node source = original_node_to_mst[e->source()];
			ogdf::node target = original_node_to_mst[e->target()];
			ogdf::edge mst_edge = mst->newEdge(source, target);
			
			mst_edge_uids->operator[](mst_edge) = graph_message->edge_uids()[e];
			mst_edge_costs[mst_edge] = og_edge_costs->operator[](e);
		}
	}

    server::graph_message spgm{std::move(mst), std::move(mst_node_uids), std::move(mst_edge_uids)};

    return std::make_pair(
        response_factory::build_response(std::unique_ptr<abstract_response>{
            new generic_response{&spgm, &mst_node_coords, &mst_edge_costs, nullptr, nullptr, nullptr,
                                 nullptr, nullptr, nullptr, status_code::OK}}),
        ogdf_time);
}

graphs::HandlerInformation mst_handler::handler_information()
{
    // add simple information
    auto information =
        mst_handler::createHandlerInformation(name(), graphs::RequestType::GENERIC);
    // add field information
    mst_handler::addFieldInformation(information, graphs::FieldInformation_FieldType_GRAPH,
                                          "Graph", "graph", true);
    mst_handler::addFieldInformation(information,
                                          graphs::FieldInformation_FieldType_EDGE_COSTS,
                                          "Edge costs", "edgeCosts", true);
    mst_handler::addFieldInformation(information,
                                          graphs::FieldInformation_FieldType_VERTEX_COORDINATES, "",
                                          "vertexCoordinates", true);

    // add result information
    mst_handler::addResultInformation(
        information, graphs::ResultInformation_HandlerReturnType_GRAPH, "graph");
    mst_handler::addResultInformation(
        information, graphs::ResultInformation_HandlerReturnType_EDGE_COSTS, "edgeCosts");
    mst_handler::addResultInformation(
        information, graphs::ResultInformation_HandlerReturnType_VERTEX_COORDINATES,
        "vertexCoordinates");
    return information;
}

std::string mst_handler::name()
{
    return "mst";
}

}  // namespace server
