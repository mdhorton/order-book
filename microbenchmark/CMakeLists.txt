set(CMAKE_PREFIX_PATH /remote/googlebenchmark/current/build)
find_package(benchmark REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(benchmark
        benchmark.cpp
)

target_link_libraries(
        benchmark
        PUBLIC
        benchmark::benchmark
        benchmark::benchmark_main
)
