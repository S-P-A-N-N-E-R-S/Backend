#pragma once
#include <pqxx/pqxx>
#include <string>

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
    std::string pw_hash;
    std::string salt;
    user_role role;

    /**
     * @brief Creates a user from a fitting pqxx::row
     * 
     * @param row 
     * @return user 
     */
    static user from_row(const pqxx::row &row);
};

}  // namespace server