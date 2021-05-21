add_executable(parsing_example parsing_example.cpp)
target_link_libraries(parsing_example PRIVATE networking)

add_executable(server server.cpp)
target_link_libraries(server PRIVATE networking)
