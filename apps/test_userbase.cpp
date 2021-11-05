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
}