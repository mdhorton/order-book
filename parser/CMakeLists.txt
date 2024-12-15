include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(
        parser
        parser.cpp
)

target_link_libraries(
        parser
)
