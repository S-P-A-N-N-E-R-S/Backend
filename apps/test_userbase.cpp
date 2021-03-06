#include <iostream>

#include <config/config.hpp>
#include <networking/exceptions.hpp>
#include <persistence/database_wrapper.hpp>
#include <persistence/user.hpp>

server::binary_data to_binary_data(const std::string &string)
{
    return server::binary_data{reinterpret_cast<const std::byte *>(string.data()), string.size()};
}

int main(int argc, const char **argv)
{
    // Parse configurations
    server::config_parser::instance().parse(argc, argv);

    std::string connection_string = server::get_db_connection_string();
    server::database_wrapper db(connection_string);

    server::user u1{-1,
                    "user1",
                    to_binary_data("12345"),
                    to_binary_data("sea salt"),
                    false,
                    server::user_role::Admin};
    server::user u2{-1,
                    "user2",
                    to_binary_data("qwert"),
                    to_binary_data("kala namak"),
                    false,
                    server::user_role::User};

    db.create_user(u1);
    db.create_user(u2);
    std::cout << "id 1: " << u1.user_id << "  id 2: " << u2.user_id << "\n";

    bool success = db.create_user(u1);

    if (success)
    {
        std::cout << "Created u1 a second time!\n";
    }
    else
    {
        std::cout << "Couldn't create u1 a second time!\n";
    }

    auto u_db = db.get_user(u1.name);

    std::cout << u_db->user_id << " " << u_db->name << " "
              << reinterpret_cast<char *>(u_db->pw_hash.data()) << " "
              << reinterpret_cast<char *>(u_db->salt.data()) << " "
              << (u_db->role == server::user_role::Admin ? "User" : "Error") << "\n";

    u_db = db.get_user(u2.user_id);

    std::cout << u_db->user_id << " " << u_db->name << " "
              << reinterpret_cast<char *>(u_db->pw_hash.data()) << " "
              << reinterpret_cast<char *>(u_db->salt.data()) << " "
              << (u_db->role == server::user_role::User ? "User" : "Error") << "\n";

    u_db = db.get_user("Not-A-User");
    if (!u_db)
    {
        std::cout << "No user Not-A-User exists\n";
    }
    else
    {
        std::cout << "User Not-A-User exists\n";
    }

    u_db = db.get_user(-1);
    if (!u_db)
    {
        std::cout << "No user with id -1 exists\n";
    }
    else
    {
        std::cout << "User with id -1 exists\n";
    }

    db.change_user_role(u1.user_id, server::user_role::User);
    db.change_user_auth(u1.user_id, "password", "stone salt");

    u_db = db.get_user(u1.user_id);

    std::cout << u_db->user_id << " " << u_db->name << " "
              << reinterpret_cast<char *>(u_db->pw_hash.data()) << " "
              << reinterpret_cast<char *>(u_db->salt.data()) << " "
              << " " << (u_db->role == server::user_role::User ? "User" : "Error") << "\n";

    if (!db.change_user_role(-1, server::user_role::Admin))
    {
        std::cout << "Could not change role of user with id -1\n";
    }
    else
    {
        std::cout << "Error: Changed role of user with id -1\n";
    }

    if (!db.change_user_auth(-1, "try", "this"))
    {
        std::cout << "Could not change auth of user with id -1\n";
    }
    else
    {
        std::cout << "Error: Changed auth of user with id -1\n";
    }

    db.delete_user(u1.user_id);
    db.delete_user(u2.user_id);

    u_db = db.get_user(u1.user_id);
    if (!u_db)
    {
        std::cout << "u1 was deleted\n";
    }
    else
    {
        std::cout << "u1 wasn't deleted\n";
    }

    u_db = db.get_user(u2.user_id);
    if (!u_db)
    {
        std::cout << "u2 was deleted\n";
    }
    else
    {
        std::cout << "u2 wasn't deleted\n";
    }
}
