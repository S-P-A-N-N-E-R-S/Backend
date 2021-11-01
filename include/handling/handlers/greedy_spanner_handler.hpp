#pragma once

#include "handling/handlers/abstract_handler.hpp"
#include "networking/requests/generic_request.hpp"

namespace server {

class greedy_spanner_handler : public abstract_handler
{
public:
    greedy_spanner_handler(std::unique_ptr<abstract_request> request);

    virtual ~greedy_spanner_handler() = default;

    virtual std::pair<graphs::ResponseContainer, long> handle() override;

    static graphs::HandlerInformation handler_information();

    static std::string name();

private:
    std::unique_ptr<generic_request> m_request;
};

}  // namespace server
