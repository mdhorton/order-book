#include <gtest/gtest.h>

#include "order_book_1/order_book.hpp"

namespace nostromo::order_book::order_book_1 {
TEST(OrderBook, bid_ask) {
   constexpr uint32_t bid_price = 400'000;
   constexpr uint32_t ask_price = 500'000;
   OrderBook book{100, 2500};
   book.OrderAdd(ItchOrderAdd{1, 1, 1, 1, 1, bid_price});
   book.OrderAdd(ItchOrderAdd{1, 1, 2, 0, 1, ask_price});
   EXPECT_EQ(book.BestBid(), bid_price);
   EXPECT_EQ(book.BestAsk(), ask_price);
}

TEST(OrderBook, bid_delete) {
   constexpr uint32_t bid_price = 400'000;
   OrderBook book{100, 2500};
   book.OrderAdd(ItchOrderAdd{1, 1, 1, 1, 1, bid_price});
   book.OrderDelete(ItchOrderDelete{1, 1, 1});
   EXPECT_EQ(book.BestBid(), 0);
}
} // nostromo::order_book::order_book_1
