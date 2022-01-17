#include <persistence/user.hpp>

namespace server {

user user::from_row(const pqxx::row &row)
{
    binary_data pw = (row["pw_hash"].is_null()) ? row["pw_hash"].as<binary_data>() : binary_data{};
    binary_data salt = (row["salt"].is_null()) ? row["salt"].as<binary_data>() : binary_data{};

    return user{row["user_id"].as<int>(),
                row["user_name"].as<std::string>(),
                std::move(pw),
                std::move(salt),
                row["blocked"].as<bool>(),
                static_cast<user_role>(row["role"].as<int>())};
}

}  // namespace server
