#pragma once

#include <nlohmann/json.hpp>
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

    nlohmann::json to_json() const;

    /**
     * @brief Creates a user from a fitting pqxx::row
     *
     * @param row
     * @return user
     */
    static user from_row(const pqxx::row &row);
};

}  // namespace server
