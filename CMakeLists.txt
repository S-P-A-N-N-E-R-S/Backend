cmake_minimum_required(VERSION 3.8)

project(server)

option(BUILD_BENCHMARKS "Build benchmarks" OFF)

set(CMAKE_CXX_STANDARD 17)

# Set up Protobuf support for CMake
find_package(Protobuf REQUIRED)
set(PROTO_PATH ${CMAKE_SOURCE_DIR}/lib/protocol/protos)

find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS system filesystem iostreams coroutine)

# Set up OpenSSL
find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    link_directories(${OPENSSL_LIBRARIES})
    include_directories(${OPENSSL_INCLUDE_DIR})
endif(OPENSSL_FOUND)

# Set up OGDF
add_subdirectory(lib/ogdf)

# GCC seems to detect many false positives for the "stringop-overread" warning.
# Since "-Werror" is enabled, we disable this warning for the OGDF. See
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99578 for more information.
if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 11.1)
    target_compile_options(OGDF PRIVATE -Wno-stringop-overread)
endif()

add_custom_target(
    Argon2 ALL
    COMMAND make
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/lib/Argon2
)

# Add sources for library
add_subdirectory(src)

# Add executable(s)
add_subdirectory(apps)

# Build benchmarks, if desired
if (BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif ()
