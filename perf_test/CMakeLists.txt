find_package(fmt REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(perf_test perf_test.cpp)
target_link_libraries(perf_test fmt::fmt)