#include <gtest/gtest.h>

#include "itch_1/order_book.hpp"

constexpr uint32_t bid_price = 400'000;
constexpr uint32_t ask_price = 500'000;
constexpr uint32_t price_divisor = 100;
constexpr uint32_t price_offset = 2500;

namespace order_book::itch_1 {
TEST(OrderBook, bid_ask) {
   OrderBook book{price_divisor, price_offset};
   book.OrderAdd(ItchOrderAdd{1, 1, 1, 1, 1, bid_price});
   EXPECT_EQ(book.GetBidCount(), 1);
   EXPECT_EQ(book.GetAskCount(), 0);
   book.OrderAdd(ItchOrderAdd{1, 1, 2, 0, 1, ask_price});
   EXPECT_EQ(book.GetBidCount(), 1);
   EXPECT_EQ(book.GetAskCount(), 1);
   EXPECT_EQ(book.BestBid(), bid_price);
   EXPECT_EQ(book.BestAsk(), ask_price);
}

TEST(OrderBook, add_delete) {
   OrderBook book{price_divisor, price_offset};
   book.OrderAdd(ItchOrderAdd{1, 1, 1, 1, 1, bid_price});
   book.OrderDelete(ItchOrderDelete{1, 1, 1});
   EXPECT_EQ(book.GetBidCount(), 0);
   EXPECT_EQ(book.BestBid(), 0);
}

TEST(OrderBook, add_cancel) {
   OrderBook book{price_divisor, price_offset};
   book.OrderAdd(ItchOrderAdd{1, 1, 1, 1, 2, bid_price});
   book.OrderCancel(ItchOrderCancel{1, 1, 1, 1});
   EXPECT_EQ(book.GetBidCount(), 1);
   const auto order = book.GetOrder(1);
   EXPECT_EQ(order->quantity, 1);
}

TEST(OrderBook, add_replace) {
   OrderBook book{price_divisor, price_offset};
   book.OrderAdd(ItchOrderAdd{1, 1, 1, 1, 1, bid_price});
   book.OrderReplace(ItchOrderReplace{1, 1, 1, 2, 1, bid_price + 1});
   EXPECT_EQ(book.GetBidCount(), 1);
   EXPECT_EQ(book.BestBid(), bid_price + 1);
}
} // order_book::itch_1
