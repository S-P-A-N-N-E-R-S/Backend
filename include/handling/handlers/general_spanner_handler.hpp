#pragma once

#include <ogdf/graphalg/SpannerModule.h>
#include <boost/core/demangle.hpp>
#include <chrono>
#include <typeinfo>
#include "handling/handlers/abstract_handler.hpp"
#include "networking/exceptions.hpp"
#include "networking/requests/generic_request.hpp"
#include "networking/responses/generic_response.hpp"
#include "networking/responses/response_factory.hpp"

namespace server {

template <class spanner_algorithm>
class general_spanner_handler : public abstract_handler
{
public:
    general_spanner_handler(std::unique_ptr<abstract_request> request);

    virtual ~general_spanner_handler() = default;

    virtual handle_return handle() override;

    static graphs::HandlerInformation handler_information();

    /**
     * @brief Constructs a consistent name from the template parameter
     *
     * @return std::string the name
     */
    static std::string name();

private:
    std::unique_ptr<generic_request> m_request;
};

template <class spanner_algorithm>
general_spanner_handler<spanner_algorithm>::general_spanner_handler(
    std::unique_ptr<abstract_request> request)
    : m_request{}
{
    if (const auto *type_check_ptr = dynamic_cast<generic_request *>(request.get());
        !type_check_ptr)
    {
        throw server::request_parse_error("general_spanner_handler: dynamic_cast failed!",
                                          request->type(), name());
    }

    m_request = std::unique_ptr<generic_request>{static_cast<generic_request *>(request.release())};
}

template <class spanner_algorithm>
handle_return general_spanner_handler<spanner_algorithm>::handle()
{
    const auto *graph_message = m_request->graph_message();
    auto &graph = graph_message->graph();

    ogdf::GraphAttributes ga(graph, ogdf::GraphAttributes::edgeDoubleWeight);
    ga.directed() = false;

    const auto *parsed_costs = m_request->edge_costs();

    for (auto edge_iterator = parsed_costs->begin(); edge_iterator != parsed_costs->end();
         edge_iterator++)
    {
        ga.doubleWeight(edge_iterator.key()) = edge_iterator.value();
    }

    double stretch = std::stod(m_request->graph_attributes().at("stretch"));

    ogdf::EdgeArray<bool> in_spanner(graph);

    auto spanner = std::make_unique<ogdf::GraphCopySimple>(graph);

    spanner_algorithm spanner_algorithm_instance;

    //Scheduler enforces time limit
    spanner_algorithm_instance.setTimelimit(-1);

    if (std::string error; !spanner_algorithm_instance.preconditionsOk(ga, stretch, error))
    {
        throw std::runtime_error("Precondition for spanner not ok: " + error);
    }

    auto start = std::chrono::high_resolution_clock::now();
    auto return_type = spanner_algorithm_instance.call(ga, stretch, *spanner, in_spanner);
    auto stop = std::chrono::high_resolution_clock::now();
    long ogdf_time = (std::chrono::duration_cast<std::chrono::microseconds>(stop - start)).count();

    const auto &og_node_uids = graph_message->node_uids();
    const auto *og_node_coords = m_request->node_coords();
    auto spanner_node_uids = std::make_unique<ogdf::NodeArray<server::uid_t>>(*spanner);
    auto spanner_node_coords = ogdf::NodeArray<node_coordinates>(*spanner);

    for (const auto spanner_node : spanner->nodes)
    {
        auto og_node = spanner->original(spanner_node);
        spanner_node_uids->operator[](spanner_node) = og_node_uids[og_node];
        spanner_node_coords[spanner_node] = og_node_coords->operator[](og_node);
    }

    const auto &og_edge_uids = graph_message->edge_uids();
    auto spanner_edge_uids = std::make_unique<ogdf::EdgeArray<server::uid_t>>(*spanner);
    auto spanner_edge_costs = ogdf::EdgeArray<double>(*spanner);

    for (const auto spanner_edge : spanner->edges)
    {
        auto og_edge = spanner->original(spanner_edge);
        spanner_edge_uids->operator[](spanner_edge) = og_edge_uids[og_edge];
        spanner_edge_costs[spanner_edge] = parsed_costs->operator[](og_edge);
    }

    server::graph_message spanner_gm{std::move(spanner), std::move(spanner_node_uids),
                                     std::move(spanner_edge_uids)};

    return {std::unique_ptr<abstract_response>{new generic_response{
                &spanner_gm, &spanner_node_coords, &spanner_edge_costs, nullptr, nullptr, nullptr,
                nullptr, nullptr, nullptr, status_code::OK}},
            ogdf_time};
}

template <class spanner_algorithm>
graphs::HandlerInformation general_spanner_handler<spanner_algorithm>::handler_information()
{
    // add simple information
    auto information =
        general_spanner_handler::createHandlerInformation(name(), graphs::RequestType::GENERIC);
    // add field information
    general_spanner_handler::addFieldInformation(
        information, graphs::FieldInformation_FieldType_GRAPH, "Graph", "graph", true);
    general_spanner_handler::addFieldInformation(information,
                                                 graphs::FieldInformation_FieldType_DOUBLE,
                                                 "Stretch factor", "graphAttributes.stretch", true);
    general_spanner_handler::addFieldInformation(information,
                                                 graphs::FieldInformation_FieldType_EDGE_COSTS,
                                                 "Edge costs", "edgeCosts", true);
    general_spanner_handler::addFieldInformation(
        information, graphs::FieldInformation_FieldType_VERTEX_COORDINATES, "", "vertexCoordinates",
        true);

    // add result information
    general_spanner_handler::addResultInformation(
        information, graphs::ResultInformation_HandlerReturnType_GRAPH, "graph");
    general_spanner_handler::addResultInformation(
        information, graphs::ResultInformation_HandlerReturnType_EDGE_COSTS, "edgeCosts");
    general_spanner_handler::addResultInformation(
        information, graphs::ResultInformation_HandlerReturnType_VERTEX_COORDINATES,
        "vertexCoordinates");
    return information;
}

template <class spanner_algorithm>
std::string general_spanner_handler<spanner_algorithm>::name()
{
    return boost::core::demangle((typeid(spanner_algorithm).name()));
}

}  // namespace server
