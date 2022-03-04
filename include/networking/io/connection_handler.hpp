#ifndef IO_SERVER_CONNECTION_HANDLER_HPP
#define IO_SERVER_CONNECTION_HANDLER_HPP

#include <map>
#include <memory>

namespace server {

/**
 * @brief Class to manage the lifetime of active/not finished <server::connection>s.
 */
template <typename CONNECTION_TYPE>
class connection_handler
{
public:
    using connection_type = CONNECTION_TYPE;

    /**
     * @brief Add a <server::connection>
     * @param identifier Used to identify the connection when trying to remove it
     * @param connection Connection to be managed. Handler takes ownership.
     */
    bool add(size_t identifier, std::unique_ptr<connection_type> connection)
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
    std::map<size_t, std::unique_ptr<connection_type>> m_store;
};

}  // namespace server

#endif
