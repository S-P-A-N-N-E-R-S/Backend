# setup tests
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/f5e592d8ee5ffb1d9af5be7f715ce3576b8bf9c4.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(pgtests test_hello_world.cpp)
target_link_libraries(pgtests gtest_main)

include(GoogleTest)
gtest_discover_tests(pgtests)
