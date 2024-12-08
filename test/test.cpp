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

TEST(OrderBook, add_delete) {
   constexpr uint32_t bid_price = 400'000;
   OrderBook book{100, 2500};
   book.OrderAdd(ItchOrderAdd{1, 1, 1, 1, 1, bid_price});
   book.OrderDelete(ItchOrderDelete{1, 1, 1});
   EXPECT_EQ(book.BestBid(), 0);
}

TEST(OrderBook, add_cancel) {
   constexpr uint32_t bid_price = 400'000;
   OrderBook book{100, 2500};
   book.OrderAdd(ItchOrderAdd{1, 1, 1, 1, 2, bid_price});
   book.OrderCancel(ItchOrderCancel{1, 1, 1, 1});
   const auto order = book.GetOrder(1);
   EXPECT_EQ(order->quantity, 1);
}

TEST(OrderBook, add_replace) {
   constexpr uint32_t bid_price = 400'000;
   OrderBook book{100, 2500};
   book.OrderAdd(ItchOrderAdd{1, 1, 1, 1, 1, bid_price});
   book.OrderReplace(ItchOrderReplace{1, 1, 1, 2, 1, bid_price + 1});
   EXPECT_EQ(book.BestBid(), bid_price + 1);
}
} // nostromo::order_book::order_book_1
