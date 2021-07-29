#pragma once

namespace server {

/**
 * Provides methods for parsing and serialization of graph attributes in OGDF representation to
 * Protobuf representation, and vice-versa. Note that all methods return `void`. The desired format
 * is contained in the second-to-last function parameter which functions as an out parameter.
 *
 * The `transformer` function is applied to the elements of the source format before writing to the
 * target format. Users can make use of this to perform desired transformations "in place", e.g.
 * converting a custom type to the format of a Protobuf class. Per default, the `transformer`
 * function returns the element as a `target_type`.
 */
namespace utils {
    template <typename source_type, typename target_type>
    void parse_node_attribute(
        const google::protobuf::RepeatedField<source_type> &rep_field, const graph_message &msg,
        ogdf::NodeArray<target_type> &node_array,
        std::function<target_type(const source_type &)> transformer =
            [](const source_type &source) -> target_type {
            return source;
        })
    {
        for (int idx = 0; idx < rep_field.size(); ++idx)
        {
            const auto &node = msg.all_nodes()[idx];
            node_array[node] = transformer(rep_field.at(idx));
        }
    }

    template <typename source_type, typename target_type>
    void parse_node_attribute(
        const google::protobuf::RepeatedPtrField<source_type> &rep_field, const graph_message &msg,
        ogdf::NodeArray<target_type> &node_array,
        std::function<target_type(const source_type &)> transformer =
            [](const source_type &source) -> target_type {
            return source;
        })
    {
        for (int idx = 0; idx < rep_field.size(); ++idx)
        {
            const auto &edge = msg.all_nodes()[idx];
            node_array[edge] = transformer(rep_field.at(idx));
        }
    }

    template <typename source_type, typename target_type>
    void parse_edge_attribute(
        const google::protobuf::RepeatedField<source_type> &rep_field, const graph_message &msg,
        ogdf::EdgeArray<target_type> &edge_array,
        std::function<target_type(const source_type &)> transformer =
            [](const source_type &source) -> target_type {
            return source;
        })
    {
        for (int idx = 0; idx < rep_field.size(); ++idx)
        {
            const auto &edge = msg.all_edges()[idx];
            edge_array[edge] = transformer(rep_field.at(idx));
        }
    }

    template <typename source_type, typename target_type>
    void parse_edge_attribute(
        const google::protobuf::RepeatedPtrField<source_type> &rep_field, const graph_message &msg,
        ogdf::EdgeArray<target_type> &edge_array,
        std::function<target_type(const source_type &)> transformer =
            [](const source_type &source) -> target_type {
            return source;
        })
    {
        for (int idx = 0; idx < rep_field.size(); ++idx)
        {
            const auto &edge = msg.all_edges()[idx];
            edge_array[edge] = transformer(rep_field.at(idx));
        }
    }

    template <typename source_type, typename target_type>
    void serialize_node_attribute(
        const ogdf::NodeArray<source_type> &node_array, const graph_message &msg,
        google::protobuf::RepeatedField<target_type> &rep_field,
        std::function<target_type(const source_type &)> transformer =
            [](const source_type &source) -> target_type {
            return source;
        })
    {
        const auto nof_nodes = msg.graph().numberOfNodes();
        const auto &all_nodes = msg.all_nodes();

        rep_field.Reserve(nof_nodes);
        rep_field.Clear();
        for (size_t idx = 0; idx < nof_nodes; ++idx)
        {
            const auto &value = node_array[all_nodes[idx]];
            rep_field.AddAlreadyReserved(transformer(value));
        }
    }

    template <typename source_type, typename target_type>
    void serialize_node_attribute(
        const ogdf::NodeArray<source_type> &node_array, const graph_message &msg,
        google::protobuf::RepeatedPtrField<target_type> &rep_field,
        std::function<target_type(const source_type &)> transformer =
            [](const source_type &source) -> target_type {
            return source;
        })
    {
        const auto nof_nodes = msg.graph().numberOfNodes();
        const auto &all_nodes = msg.all_nodes();

        rep_field.Reserve(nof_nodes);
        rep_field.Clear();
        for (size_t idx = 0; idx < nof_nodes; ++idx)
        {
            const auto &value = node_array[all_nodes[idx]];
            rep_field.Add(transformer(value));
        }
    }

    template <typename source_type, typename target_type>
    void serialize_edge_attribute(
        const ogdf::EdgeArray<source_type> &edge_array, const graph_message &msg,
        google::protobuf::RepeatedField<target_type> &rep_field,
        std::function<target_type(const source_type &)> transformer =
            [](const source_type &source) -> target_type {
            return source;
        })
    {
        const auto nof_edges = msg.graph().numberOfEdges();
        const auto &all_edges = msg.all_edges();

        rep_field.Reserve(nof_edges);
        rep_field.Clear();
        for (size_t idx = 0; idx < nof_edges; ++idx)
        {
            const auto &value = edge_array[all_edges[idx]];
            rep_field.AddAlreadyReserved(transformer(value));
        }
    }

    template <typename source_type, typename target_type>
    void serialize_edge_attribute(
        const ogdf::EdgeArray<source_type> &edge_array, const graph_message &msg,
        google::protobuf::RepeatedPtrField<target_type> &rep_field,
        std::function<target_type(const source_type &)> transformer =
            [](const source_type &source) -> target_type {
            return source;
        })
    {
        const auto nof_edges = msg.graph().numberOfEdges();
        const auto &all_edges = msg.all_edges();

        rep_field.Reserve(nof_edges);
        rep_field.Clear();
        for (size_t idx = 0; idx < nof_edges; ++idx)
        {
            const auto &value = edge_array[all_edges[idx]];
            rep_field.Add(transformer(value));
        }
    }
}  // namespace utils
}  // namespace server
