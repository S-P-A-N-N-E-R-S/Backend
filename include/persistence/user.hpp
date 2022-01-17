#pragma once

#include <boost/json.hpp>
#include <pqxx/pqxx>
#include <string>

#include <persistence/database_wrapper.hpp>

namespace server {

enum class user_role { User, Admin };

/**
 * @brief Struct to represent user data
 *
 */
struct user {
    /**
     * @brief Id in the databse. Can be left arbitrarily if object is used to create
     * user in the database
     *
     */
    int user_id;
    std::string name;
    binary_data pw_hash;
    binary_data salt;
    bool blocked;
    user_role role;

    boost::json::object to_json() const
    {
        boost::json::object json_user{};
        json_user["id"] = user_id;
        json_user["name"] = name;
        json_user["blocked"] = blocked;
        json_user["role"] = static_cast<int64_t>(role);
        return json_user;
    }

    /**
     * @brief Creates a user from a fitting pqxx::row
     *
     * @param row
     * @return user
     */
    static user from_row(const pqxx::row &row);
};

}  // namespace server
