include_directories(
        ${PROJECT_SOURCE_DIR}/src
)

set(ORDER_BOOK_SOURCE_FILES
        main.cpp
        common.hpp
        itch/prices/prices.hpp
        itch/itch.hpp
        itch/itch_raw.hpp
        itch/v02/order_book.hpp
        itch/v03/order_book.hpp
        itch/v04/order_book.hpp
)

add_executable(order_book ${ORDER_BOOK_SOURCE_FILES})
target_link_libraries(order_book)
