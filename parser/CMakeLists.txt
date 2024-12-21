find_package(ZLIB REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(
        itch_raw_parser
        itch_raw_parser.cpp
)

target_link_libraries(
        itch_raw_parser
        ${ZLIB_LIBRARIES}
        ${EXTERNAL_DEPENDENCY_DIR}/boost/current/install/lib/libboost_iostreams.so
)

add_executable(
        metadata_creator
        metadata_creator.cpp
)

target_link_libraries(
        metadata_creator
)