find_package(benchmark REQUIRED PATHS ${EXTERNAL_DEPENDENCY_DIR}/googlebenchmark/current/build)

include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(
        microbenchmark
        microbenchmark.cpp
)

target_link_libraries(
        microbenchmark
        PUBLIC
        benchmark::benchmark
        benchmark::benchmark_main
)
