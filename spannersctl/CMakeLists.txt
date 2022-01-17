project(spannersctl)

# Create server library
set(CLI_APP spannersctl)

set(CLI_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/io/io.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/subcommands/constants.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/subcommands/job.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/subcommands/scheduler.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/subcommands/user.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/util/join.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/util/json.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/util/span.hpp
    # TODO
)

set(CLI_SOURCES
    src/io/io.cpp
    src/main.cpp
    src/subcommands/job.cpp
    src/subcommands/scheduler.cpp
    src/subcommands/user.cpp
    src/util/join.cpp
    src/util/json.cpp
    # TODO
)

add_executable(${CLI_APP} ${CLI_HEADER} ${CLI_SOURCES})

target_include_directories(${CLI_APP} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(${CLI_APP} PUBLIC ${SERVER_INCLUDE_DIRS})
target_include_directories(${CLI_APP} PUBLIC ${PROTOBINDING_PATH})

target_link_libraries(${CLI_APP} PUBLIC ${SERVER_LIB})
target_link_libraries(${CLI_APP} PUBLIC ${Boost_LIBRARIES})
target_link_libraries(${CLI_APP} PUBLIC ${PQXX_LIB})
