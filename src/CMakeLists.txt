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

#Suggestion: Networking and Handling do not need different librarys, we could combine them into one later

#Create networking library
set(NETWORKING_NAME networking)

set(NETWORKING_HEADERS
    ${CMAKE_SOURCE_DIR}/include/networking/exceptions.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/io/io_server.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/io/connection_handler.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/io/connection.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/messages/graph_message.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/messages/node_coordinates.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/responses/abstract_response.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/responses/available_handlers_response.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/responses/generic_response.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/responses/shortest_path_response.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/responses/status_code.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/responses/response_factory.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/responses/response_type.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/requests/abstract_request.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/requests/generic_request.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/requests/shortest_path_request.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/requests/request_factory.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/requests/request_type.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/utils.hpp
    )

set(NETWORKING_SOURCES
    io/io_server.cpp
    io/connection.cpp
    messages/graph_message.cpp
    messages/node_coordinates.cpp
    responses/abstract_response.cpp
    responses/available_handlers_response.cpp
    responses/generic_response.cpp
    responses/shortest_path_response.cpp
    responses/response_factory.cpp
    requests/abstract_request.cpp
    requests/generic_request.cpp
    requests/shortest_path_request.cpp
    requests/request_factory.cpp
    )

set(networking_files "${NETWORKING_HEADERS};${NETWORKING_SOURCES};${proto_headers};${proto_sources}")

add_library(${NETWORKING_NAME} STATIC "${networking_files}")
target_include_directories(${NETWORKING_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_include_directories(${NETWORKING_NAME} PUBLIC ${OGDF_INCLUDE_DIRS})
target_include_directories(${NETWORKING_NAME} PRIVATE ${Protobuf_INCLUDE_DIRS})
target_include_directories(${NETWORKING_NAME} PUBLIC "${PROTOBINDING_PATH}")

target_link_libraries(${NETWORKING_NAME} PUBLIC OGDF)
target_link_libraries(${NETWORKING_NAME} PRIVATE ${Protobuf_LIBRARIES})
target_link_libraries(${NETWORKING_NAME} PUBLIC ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(${NETWORKING_NAME} PRIVATE ${Boost_LIBRARIES})


#Create handling library
set(HANDLING_NAME handling)

set(HANDLING_HEADERS
    ${CMAKE_SOURCE_DIR}/include/handling/handler_factory.hpp
    ${CMAKE_SOURCE_DIR}/include/handling/handler_proxy.hpp
    ${CMAKE_SOURCE_DIR}/include/handling/handlers/abstract_handler.hpp
    ${CMAKE_SOURCE_DIR}/include/handling/handlers/dijkstra_handler.hpp
)

set(HANDLING_SOURCES
    ${CMAKE_SOURCE_DIR}/src/handling/handler_factory.cpp
    ${CMAKE_SOURCE_DIR}/src/handling/handler_proxy.cpp
    ${CMAKE_SOURCE_DIR}/src/handling/handlers/abstract_handler.cpp
    ${CMAKE_SOURCE_DIR}/src/handling/handlers/dijkstra_handler.cpp
)

set(handling_files "${HANDLING_HEADERS};${HANDLING_SOURCES};${proto_headers};${proto_sources}")
add_library(${HANDLING_NAME} STATIC "${handling_files}")
target_include_directories(${HANDLING_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/include/handling")
target_include_directories(${HANDLING_NAME} PUBLIC ${OGDF_INCLUDE_DIRS})
target_include_directories(${HANDLING_NAME} PRIVATE ${Protobuf_INCLUDE_DIRS})
target_include_directories(${HANDLING_NAME} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(${HANDLING_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)


target_link_libraries(${HANDLING_NAME} PUBLIC OGDF)
target_link_libraries(${HANDLING_NAME} PRIVATE ${Protobuf_LIBRARIES})
target_link_libraries(${HANDLING_NAME} PUBLIC ${NETWORKING_NAME})


#Create scheduling library
set(SCHEDULING_NAME scheduling)

set(SCHEDULING_HEADERS
    ${CMAKE_SOURCE_DIR}/include/scheduler/process_flags.hpp
    ${CMAKE_SOURCE_DIR}/include/scheduler/scheduler.hpp
)

set(SCHEDULING_SOURCES
    ${CMAKE_SOURCE_DIR}/src/scheduler/scheduler.cpp
)

set(scheduling_files "${SCHEDULING_HEADERS};${SCHEDULING_SOURCES};${proto_headers};${proto_sources}")
add_library(${SCHEDULING_NAME} STATIC "${scheduling_files}")
target_include_directories(${SCHEDULING_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/include/scheduler")
target_include_directories(${SCHEDULING_NAME} PUBLIC ${OGDF_INCLUDE_DIRS})
target_include_directories(${SCHEDULING_NAME} PRIVATE ${Protobuf_INCLUDE_DIRS})
target_include_directories(${SCHEDULING_NAME} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(${SCHEDULING_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)


target_link_libraries(${SCHEDULING_NAME} PUBLIC OGDF)
target_link_libraries(${SCHEDULING_NAME} PRIVATE ${Protobuf_LIBRARIES})
target_link_libraries(${SCHEDULING_NAME} PUBLIC ${NETWORKING_NAME})

#create database library
set(DATABASE_NAME persistence)

set(DATABASE_HEADERS
    ${CMAKE_SOURCE_DIR}/include/persistence/database_wrapper.hpp
)

set(DATABASE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/persistence/database_wrapper.cpp
)

set(database_files "${DATABASE_HEADERS};${DATABASE_SOURCES};${proto_headers};${proto_sources}")
add_library(${DATABASE_NAME} STATIC "${database_files}")
target_include_directories(${DATABASE_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/include/scheduler")
target_include_directories(${DATABASE_NAME} PUBLIC ${OGDF_INCLUDE_DIRS})
target_include_directories(${DATABASE_NAME} PRIVATE ${Protobuf_INCLUDE_DIRS})
target_include_directories(${DATABASE_NAME} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(${DATABASE_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)

find_library(PQXX_LIB pqxx REQUIRED)
find_library(PQ_LIB pq REQUIRED)
target_link_libraries(${DATABASE_NAME} PRIVATE ${PQXX_LIB})
target_link_libraries(${DATABASE_NAME} PUBLIC OGDF)
target_link_libraries(${DATABASE_NAME} PRIVATE ${Protobuf_LIBRARIES})
target_link_libraries(${DATABASE_NAME} PUBLIC ${NETWORKING_NAME})


# Build handler_process
add_executable(handler_process ${CMAKE_SOURCE_DIR}/src/handling/handler_process.cpp)
target_link_libraries(handler_process PRIVATE handling)
target_link_libraries(handler_process PRIVATE scheduling)
target_link_libraries(handler_process PRIVATE persistence)
