#pragma once

#include <memory>

#include "networking/messages/graph_message.hpp"
#include "networking/requests/abstract_request.hpp"

namespace server {

class shortest_path_request : public abstract_request
{
public:
    shortest_path_request(const graphs::ShortPathRequest &proto_request);
    virtual ~shortest_path_request() = default;

    const server::graph_message *graph_message();
    std::unique_ptr<server::graph_message> take_graph_message();
    uid_t start_index() const;
    uid_t end_index() const;

private:
    std::unique_ptr<server::graph_message> m_graph_message;
    uid_t m_start_index;
    uid_t m_end_index;
};

}  // namespace server
