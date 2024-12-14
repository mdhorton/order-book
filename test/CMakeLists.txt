find_package(GTest REQUIRED PATHS /remote/googletest/current/install)

include_directories(${PROJECT_SOURCE_DIR}/src)

enable_testing()

add_executable(
        test
        test.cpp
)

target_link_libraries(
        test
        PUBLIC
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test)