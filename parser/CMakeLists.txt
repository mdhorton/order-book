find_package(fmt REQUIRED)
find_package(ZLIB REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(itch_raw_parser itch_raw_parser.cpp)
target_link_libraries(
        itch_raw_parser
        fmt::fmt
        ${ZLIB_LIBRARIES}
        ${EXTERNAL_DEPENDENCY_DIR}/boost/current/install/lib/libboost_iostreams.so
)

add_executable(itch_preprocessor itch_preprocessor.cpp)
target_link_libraries(itch_preprocessor fmt::fmt)
