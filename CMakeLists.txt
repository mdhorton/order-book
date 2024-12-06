cmake_minimum_required(VERSION 3.30)

project(order_book)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -fno-rtti")

# https://github.com/abseil/abseil-cpp/blob/master/absl/copts/copts.py
add_compile_options(
        -Wall
        -Wextra
        -Wcast-qual
        -Wconversion-null
        -Wformat-security
        -Wmissing-declarations
        -Wnon-virtual-dtor
        -Woverlength-strings
        -Wpointer-arith
        -Wundef
        -Wunused-local-typedefs
        -Wunused-result
        -Wvarargs
        -Wvla
        -Wwrite-strings
        -DNOMINMAX
        -Wsign-compare
)

add_subdirectory(microbenchmark)
add_subdirectory(src)
add_subdirectory(test)
