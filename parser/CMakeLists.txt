find_package(ZLIB REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(
        itch_raw_parser
        itch_raw_parser.cpp
)

target_link_libraries(
        itch_raw_parser
        ${ZLIB_LIBRARIES}
        /remote/boost/current/install/lib/libboost_iostreams.so
)
