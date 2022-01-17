find_library(PQXX_LIB pqxx REQUIRED)
find_library(PQ_LIB pq REQUIRED)

# Create protobuf files
file(GLOB_RECURSE PROTO_FILES "${PROTO_PATH}/*.proto")
file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/generated" PROTOBINDING_PATH)
file(MAKE_DIRECTORY ${PROTOBINDING_PATH})

foreach(proto ${PROTO_FILES})
    file(TO_NATIVE_PATH ${proto} proto_native)

    string(ASCII 27 Esc)
    message("${Esc}[1;34mRunning protobuf compiler on ${proto_native} ...")

    execute_process(COMMAND ${Protobuf_PROTOC_EXECUTABLE} --proto_path=${PROTO_PATH} --cpp_out=${PROTOBINDING_PATH} ${proto_native} RESULT_VARIABLE rv)

    if(${rv})
        message(FATAL_ERROR "protobuf compiler error for file ${proto_native}: ${rv}")
    endif()
endforeach()

file(GLOB_RECURSE proto_headers "${PROTOBINDING_PATH}/*.pb.h")
file(GLOB_RECURSE proto_sources "${PROTOBINDING_PATH}/*.pb.cc")

# Create server library
set(SERVER_NAME server-lib)

set(SERVER_HEADERS
    ${CMAKE_SOURCE_DIR}/include/config/config.hpp
    ${CMAKE_SOURCE_DIR}/include/handling/handler_factory.hpp
    ${CMAKE_SOURCE_DIR}/include/handling/handler_utilities.hpp
    ${CMAKE_SOURCE_DIR}/include/handling/handlers/abstract_handler.hpp
    ${CMAKE_SOURCE_DIR}/include/handling/handlers/dijkstra_handler.hpp
    ${CMAKE_SOURCE_DIR}/include/handling/handlers/general_spanner_handler.hpp
    ${CMAKE_SOURCE_DIR}/include/handling/handlers/kruskal_handler.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/exceptions.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/io/io_server.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/io/client_server.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/io/management_server.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/io/connection_handler.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/io/client_connection.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/io/request_handling.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/messages/graph_message.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/messages/node_coordinates.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/messages/meta_data.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/responses/abstract_response.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/responses/available_handlers_response.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/responses/generic_response.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/responses/shortest_path_response.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/responses/status_code.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/responses/new_job_response.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/responses/origin_graph_response.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/responses/response_factory.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/responses/response_type.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/responses/status_response.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/requests/abstract_request.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/requests/generic_request.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/requests/shortest_path_request.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/requests/request_factory.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/requests/request_type.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/utils.hpp
    ${CMAKE_SOURCE_DIR}/include/persistence/database_wrapper.hpp
    ${CMAKE_SOURCE_DIR}/include/persistence/user.hpp
    ${CMAKE_SOURCE_DIR}/include/scheduler/process_flags.hpp
    ${CMAKE_SOURCE_DIR}/include/scheduler/scheduler.hpp
    ${CMAKE_SOURCE_DIR}/include/auth/auth_utils.hpp
)

set(SERVER_SOURCES
    config/config.cpp
    handling/handler_utilities.cpp
    handling/register_handlers.cpp
    handling/handlers/abstract_handler.cpp
    handling/handlers/dijkstra_handler.cpp
    handling/handlers/kruskal_handler.cpp
    io/io_server.cpp
    io/client_server.cpp
    io/management_server.cpp
    io/client_connection.cpp
    io/request_handling.cpp
    messages/graph_message.cpp
    messages/node_coordinates.cpp
    persistence/database_wrapper.cpp
    persistence/user.cpp
    responses/abstract_response.cpp
    responses/available_handlers_response.cpp
    responses/generic_response.cpp
    responses/shortest_path_response.cpp
    responses/new_job_response.cpp
    responses/origin_graph_response.cpp
    responses/response_factory.cpp
    responses/status_response.cpp
    requests/abstract_request.cpp
    requests/generic_request.cpp
    requests/shortest_path_request.cpp
    requests/request_factory.cpp
    scheduler/scheduler.cpp
    auth/auth_utils.cpp
)

set(server_files
    ${SERVER_HEADERS}
    ${SERVER_SOURCES}
    ${proto_headers}
    ${proto_sources}
)

add_library(${SERVER_NAME} STATIC ${server_files})
add_dependencies(${SERVER_NAME} Argon2)

target_include_directories(${SERVER_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_include_directories(${SERVER_NAME} PUBLIC ${OGDF_INCLUDE_DIRS})
target_include_directories(${SERVER_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/lib/Argon2/include")
target_include_directories(${SERVER_NAME} PUBLIC ${PROTOBINDING_PATH})
target_include_directories(${SERVER_NAME} PRIVATE ${Protobuf_INCLUDE_DIRS})

target_link_libraries(${SERVER_NAME} PUBLIC OGDF)
target_link_libraries(${SERVER_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/lib/Argon2/libargon2.a")
target_link_libraries(${SERVER_NAME} PUBLIC ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(${SERVER_NAME} PUBLIC ${Boost_LIBRARIES})
target_link_libraries(${SERVER_NAME} INTERFACE ${PQXX_LIB})
target_link_libraries(${SERVER_NAME} PRIVATE ${Protobuf_LIBRARIES})
target_link_libraries(${SERVER_NAME} PRIVATE ${OPENSSL_LIBRARIES})

# Build handler_process
add_executable(handler_process handling/handler_process.cpp)
target_link_libraries(handler_process PRIVATE ${SERVER_NAME})

# Set a preprocessor macro to disable TLS encryption
OPTION(UNENCRYPTED_CONNECTION "Disable TLS encryption" OFF)
IF(UNENCRYPTED_CONNECTION)
    target_compile_definitions(${SERVER_NAME} PUBLIC SPANNERS_UNENCRYPTED_CONNECTION)
ENDIF(UNENCRYPTED_CONNECTION)
