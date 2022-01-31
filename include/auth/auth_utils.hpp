#pragma once

#include <string>

#include <persistence/database_wrapper.hpp>

namespace server {

namespace auth_utils {

    /**
     * @brief Checks if a given password produces a given hash using a given salt.
     *        Used for password checking.
     *
     * @param password Constant reference to a string containing a password
     * @param salt Constant reference to the salt used for the hashing operation
     * @param correct_hash Constant reference to the correct hash from the database to hash the
     *        password
     * @return bool True if password is correct, else false is returned
     */
    bool check_password(const std::string &password, const binary_data &salt,
                        const binary_data &correct_hash);

    /**
     * @brief Produces a hash and a salt for a given password string.
     *
     * @param pw Constant reference to a clear password string to be hashed
     * @param hashed_pw Reference to the buffer the hash will be stored in
     * @param salt Reference to the buffer the salt will be stored in
     * @return bool True if operation was successful, else false is returned
     */
    bool hash_password(const std::string &pw, binary_data &hashed_pw, binary_data &salt);

}  // namespace auth_utils

}  // namespace server
