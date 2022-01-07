#pragma once

#include "handling/handlers/abstract_handler.hpp"
#include "networking/requests/generic_request.hpp"

namespace server {

class kruskal_handler : public abstract_handler
{
public:
    static std::string name();

    static graphs::HandlerInformation handler_information();

    kruskal_handler(std::unique_ptr<abstract_request> request);

    virtual handle_return handle() override;

private:
    std::unique_ptr<generic_request> m_request;
};

}  // namespace server
