include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(perf_test_v01 perf_test_v01.cpp)
target_link_libraries(perf_test_v01)

add_executable(perf_test_v02 perf_test_v02.cpp)
target_link_libraries(perf_test_v02)