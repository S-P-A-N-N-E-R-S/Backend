#ifndef IO_SERVER_CONNECTION_HANDLER_HPP
#define IO_SERVER_CONNECTION_HANDLER_HPP

#include <map>
#include <memory>

#include <networking/io/connection.hpp>

namespace server {

/**
 * @brief Class to manage the lifetime of active/not finished <server::connection>s.
 */
class connection_handler
{
public:
    /**
     * @brief Add a <server::connection>
     * @param identifier Used to identify the connection when trying to remove it
     * @param connection Connection to be managed. Handler takes ownership.
     */
    bool add(size_t identifier, std::unique_ptr<connection> connection)
    {
        if (auto [conn_it, success] = m_store.insert({identifier, std::move(connection)}); success)
        {
            conn_it->second->handle();
            return true;
        }
        return false;
    }

    /**
     * @brief Remove a <server::connection>
     * @param identifier Identifier of a previously added connection.
     */
    bool remove(size_t identifier)
    {
        if (const auto &conn_it = m_store.find(identifier); conn_it != m_store.end())
        {
            m_store.erase(conn_it);
            return true;
        }
        return false;
    }

private:
    std::map<size_t, std::unique_ptr<connection>> m_store;
};

}  // namespace server

#endif
