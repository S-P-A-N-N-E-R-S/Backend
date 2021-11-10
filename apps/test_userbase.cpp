#include <iostream>

#include <networking/exceptions.hpp>
#include <persistence/database_wrapper.hpp>

int main(int argc, const char **argv)
{
    std::string connection_string = "host=localhost port=5432 user= spanner_user dbname=spanner_db "
                                    "password=pwd connect_timeout=10";
    server::database_wrapper db(connection_string);

    server::user u1{-1, "user1", "12345", "sea salt", server::user_role::Admin};
    server::user u2{-1, "user2", "qwert", "kala namak", server::user_role::User};

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

    std::cout << u_db->user_id << " " << u_db->name << " " << u_db->pw_hash << " " << u_db->salt
              << " " << (u_db->role == server::user_role::Admin ? "Admin" : "Error") << "\n";

    u_db = db.get_user(u2.user_id);

    std::cout << u_db->user_id << " " << u_db->name << " " << u_db->pw_hash << " " << u_db->salt
              << " " << (u_db->role == server::user_role::User ? "User" : "Error") << "\n";

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

    std::cout << u_db->user_id << " " << u_db->name << " " << u_db->pw_hash << " " << u_db->salt
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