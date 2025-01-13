include_directories(${PROJECT_SOURCE_DIR}/src)

set(ORDER_BOOK_SOURCE_FILES
        main.cpp
        common.hpp
        itch/itch.hpp
        itch/itch_raw.hpp
        itch/v02/order_book.hpp
        itch/v03/order_book.hpp
        itch/v05/order_book.hpp
        itch/v06/order_book.hpp
        itch/v07/order_book.hpp
        itch/v08/order_book.hpp
        itch/v09/order_book.hpp
        itch/v10/order_book.hpp
        itch/v11/order_book.hpp
        itch/v12/order_book.hpp
        itch/v13/order_book.hpp
        itch/v14/order_book.hpp
        itch/v15/order_book.hpp
)

add_executable(order_book ${ORDER_BOOK_SOURCE_FILES})
target_link_libraries(order_book)
