cmake_minimum_required(VERSION 3.8)

project(server)
set(CMAKE_CXX_STANDARD 17)

# Set up Protobuf support for CMake
find_package(Protobuf REQUIRED)
set(PROTO_PATH ${CMAKE_SOURCE_DIR}/lib/protocol/protos)

find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS  iostreams)

set(PROTO_FILES ${PROTO_PATH}/GraphData.proto
    # ${PROTO_PATH}/xd.proto
    # Put more files here
    )

# Set up OGDF
add_subdirectory(lib/ogdf)

# Add sources for library
add_subdirectory(src)

# Add executable(s)
add_subdirectory(apps)
