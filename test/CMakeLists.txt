set(CMAKE_PREFIX_PATH /remote/googletest/googletest-1.15.2/install)
find_package(GTest REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/src)

enable_testing()

add_executable(
        test
        test.cpp
)

target_link_libraries(
        test
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test)