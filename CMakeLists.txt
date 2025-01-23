cmake_minimum_required(VERSION 3.22)

project(order_book VERSION 1.0)

set(INTERNAL_DEPENDENCY_DIR /home/mdhorton/CLionProjects)
set(EXTERNAL_DEPENDENCY_DIR /remote)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_VERBOSE_MAKEFILE ON)

SET(CMAKE_CXX_FLAGS "-fno-rtti -march=native")
#SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -fprofile-generate")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -fprofile-use")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -fno-omit-frame-pointer -UNDEBUG")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fno-omit-frame-pointer -fsanitize=address")

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

include_directories(include)

include_directories(${INTERNAL_DEPENDENCY_DIR}/nostromo/src)

# eliminate boost -Wundef warnings.
add_compile_definitions(BOOST_LIBSTDCXX_VERSION_WORKAROUND_GUARD=0)
include_directories(${EXTERNAL_DEPENDENCY_DIR}/boost/current/install/include)

add_subdirectory(microbenchmark)
add_subdirectory(perf_test)
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(parser)
