#include <persistence/user.hpp>

namespace server {

user user::from_row(const pqxx::row &row)
{
    return user{row[0].as<int>(), row[1].as<std::string>(), row[2].as<binary_data>(),
                row[3].as<binary_data>(), static_cast<user_role>(row[4].as<int>())};
}

}  // namespace server
