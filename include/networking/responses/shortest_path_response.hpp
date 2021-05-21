#pragma once

#include <memory>

#include "networking/messages/graph_message.hpp"
#include "networking/responses/abstract_response.hpp"

namespace server {

class shortest_path_response : public abstract_response
{
public:
    shortest_path_response(std::unique_ptr<server::graph_message> shortest_path,
                           status_code status);
    virtual ~shortest_path_response() = default;

    const graphs::Graph *proto_graph();
    std::unique_ptr<graphs::Graph> take_proto_graph();

private:
    std::unique_ptr<graphs::Graph> m_proto_graph;
};

}  // namespace server
