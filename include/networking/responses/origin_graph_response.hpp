#pragma once

#include "networking/responses/abstract_response.hpp"

#include "origin_graph.pb.h"

namespace server {

class origin_graph_response : public abstract_response
{
public:
    origin_graph_response(graphs::OriginGraphResponse &&origin_graph_response, status_code status);
    virtual ~origin_graph_response() = default;

    graphs::OriginGraphResponse take_origin_graph_response();

private:
    graphs::OriginGraphResponse m_origin_graph_response;
};

}  // namespace server
