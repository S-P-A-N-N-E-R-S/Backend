project(server-benchmark)

find_package(benchmark REQUIRED)

set(BENCHMARK_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/graph_message.cpp
)

add_executable(${PROJECT_NAME} ${BENCHMARK_SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE server-lib)
target_link_libraries(${PROJECT_NAME} PRIVATE benchmark::benchmark benchmark::benchmark_main)
