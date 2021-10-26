#include "handling/handlers/convex_hull_handler.hpp"

#include <ogdf/graphalg/ConvexHull.h>
#include <chrono>
#include "networking/exceptions.hpp"
#include "networking/responses/generic_response.hpp"
#include "networking/responses/response_factory.hpp"

namespace server {

convex_hull_handler::convex_hull_handler(std::unique_ptr<abstract_request> request)
    : m_request{}
{
    if (const auto *type_check_ptr = dynamic_cast<generic_request *>(request.get());
        !type_check_ptr)
    {
        throw server::request_parse_error("convex_hull_handler: dynamic_cast failed!",
                                          request->type(), "dijkstra");
    }

    m_request = std::unique_ptr<generic_request>{static_cast<generic_request *>(request.release())};
}

std::pair<graphs::ResponseContainer, long> convex_hull_handler::handle()
{
    const auto *graph_message = m_request->graph_message();
    auto &graph = graph_message->graph();

    std::vector<ogdf::DPoint> points;
    for (auto &coord : *(m_request->node_coords()))
    {
        points.emplace_back(coord.m_x, coord.m_y);
    }

    auto start = std::chrono::high_resolution_clock::now();

    auto hull = ogdf::ConvexHull().call(points);

    auto stop = std::chrono::high_resolution_clock::now();
    long ogdf_time = (std::chrono::duration_cast<std::chrono::microseconds>(stop - start)).count();

    auto g = std::make_unique<ogdf::Graph>();
    auto coords = ogdf::NodeArray<node_coordinates>(*g);
    auto node_uids = std::make_unique<ogdf::NodeArray<uid_t>>(*g);
    auto edge_uids = std::make_unique<ogdf::EdgeArray<server::uid_t>>(*g);

    if (hull.size() > 0)
    {
        uid_t id = 0;
        auto it = hull.begin();

        ogdf::node first = g->newNode();
        coords[first] = node_coordinates{(*it).m_x, (*it).m_y, 0.0};
        ogdf::node previous = first;

        while (++it != hull.end())
        {
            ogdf::node actual = g->newNode();
            coords[actual] = {(*it).m_x, (*it).m_y, 0.0};
            ogdf::edge e = g->newEdge(previous, actual);

            node_uids->operator[](previous) = id;
            edge_uids->operator[](e) = id++;

            previous = actual;
        }

        ogdf::edge e = g->newEdge(previous, first);
        node_uids->operator[](previous) = id;
        edge_uids->operator[](e) = id++;
    }

    server::graph_message gm{std::move(g), std::move(node_uids), std::move(edge_uids)};

    return std::make_pair(response_factory::build_response(std::unique_ptr<abstract_response>{
                              new generic_response{&gm, &coords, nullptr, nullptr, nullptr, nullptr,
                                                   nullptr, nullptr, nullptr, status_code::OK}}),
                          ogdf_time);
}

graphs::HandlerInformation convex_hull_handler::handler_information()
{
    // add simple information
    auto information =
        convex_hull_handler::createHandlerInformation(name(), graphs::RequestType::GENERIC);
    // add field information
    convex_hull_handler::addFieldInformation(information, graphs::FieldInformation_FieldType_GRAPH,
                                             "Graph", "graph", true);
    convex_hull_handler::addFieldInformation(information,
                                             graphs::FieldInformation_FieldType_VERTEX_COORDINATES,
                                             "", "vertexCoordinates", true);

    // add result information
    convex_hull_handler::addResultInformation(
        information, graphs::ResultInformation_HandlerReturnType_GRAPH, "graph");
    convex_hull_handler::addResultInformation(
        information, graphs::ResultInformation_HandlerReturnType_VERTEX_COORDINATES,
        "vertexCoordinates");
    return information;
}

std::string convex_hull_handler::name()
{
    return "convex hull";
}

}  // namespace server
