set(NETWORKING_NAME networking)

set(NETWORKING_HEADERS
    ${CMAKE_SOURCE_DIR}/include/networking/io/io_server.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/io/connection_handler.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/io/connection.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/messages/graph_message.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/responses/abstract_response.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/responses/shortest_path_response.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/responses/status_code.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/responses/response_factory.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/responses/response_type.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/requests/abstract_request.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/requests/shortest_path_request.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/requests/request_factory.hpp
    ${CMAKE_SOURCE_DIR}/include/networking/requests/request_type.hpp
    )

set(NETWORKING_SOURCES
    io/io_server.cpp
    io/connection.cpp
    messages/graph_message.cpp
    responses/abstract_response.cpp
    responses/shortest_path_response.cpp
    responses/response_factory.cpp
    requests/abstract_request.cpp
    requests/shortest_path_request.cpp
    requests/request_factory.cpp
    )

protobuf_generate_cpp(proto_sources proto_headers ${PROTO_FILES})
set(networking_files "${NETWORKING_HEADERS};${NETWORKING_SOURCES};${proto_headers};${proto_sources}")

add_library(${NETWORKING_NAME} STATIC "${networking_files}")
target_include_directories(${NETWORKING_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_include_directories(${NETWORKING_NAME} PUBLIC ${OGDF_INCLUDE_DIRS})
target_include_directories(${NETWORKING_NAME} PRIVATE ${Protobuf_INCLUDE_DIRS})
target_include_directories(${NETWORKING_NAME} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(${NETWORKING_NAME} PUBLIC OGDF)
target_link_libraries(${NETWORKING_NAME} PRIVATE ${Protobuf_LIBRARIES})
target_link_libraries(${NETWORKING_NAME} PUBLIC ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(${NETWORKING_NAME} PRIVATE ${Boost_LIBRARIES})
