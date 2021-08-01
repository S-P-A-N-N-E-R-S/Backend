cmake_minimum_required(VERSION 3.8)

project(server)
set(CMAKE_CXX_STANDARD 17)

# Set up Protobuf support for CMake
find_package(Protobuf REQUIRED)
set(PROTO_PATH ${CMAKE_SOURCE_DIR}/lib/protocol/protos)

find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS iostreams coroutine)

set(PROTO_FILES
    ${PROTO_PATH}/meta.proto
    ${PROTO_PATH}/shared_messages.proto
    ${PROTO_PATH}/container.proto
    ${PROTO_PATH}/graph.proto
    ${PROTO_PATH}/shortest_path.proto
    # Put more files here
    )

# Set up OGDF
add_subdirectory(lib/ogdf)

# GCC seems to detect many false positives for the "stringop-overread" warning.
# Since "-Werror" is enabled, we disable this warning for the OGDF. See
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99578 for more information.
if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 11.1)
    target_compile_options(OGDF PRIVATE -Wno-stringop-overread)
endif()

# Add sources for library
add_subdirectory(src)

# Add executable(s)
add_subdirectory(apps)
