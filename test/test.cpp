#include <gtest/gtest.h>

#include "itch/v02/order_book.hpp"

constexpr uint32_t BID_PRICE = 400'000;
constexpr uint32_t ASK_PRICE = 500'000;

namespace order_book::itch::v02 {

TEST(OrderBook, bid_ask) {
   std::vector<Order> orders = {Order{}, Order{}};
   OrderBook book{orders, {BID_PRICE}, {ASK_PRICE}};
   book.OrderAdd(ItchOrderAdd{1, 1, 0, 1, 1, BID_PRICE});
   EXPECT_EQ(book.BidCount(), 1);
   EXPECT_EQ(book.AskCount(), 0);
   book.OrderAdd(ItchOrderAdd{1, 1, 1, 0, 1, ASK_PRICE});
   EXPECT_EQ(book.BidCount(), 1);
   EXPECT_EQ(book.AskCount(), 1);
   EXPECT_EQ(book.BestBid(), BID_PRICE);
   EXPECT_EQ(book.BestAsk(), ASK_PRICE);
}

TEST(OrderBook, add_delete) {
   std::vector<Order> orders = {Order{}};
   OrderBook book{orders, {BID_PRICE}, {}};
   book.OrderAdd(ItchOrderAdd{1, 1, 0, 1, 1, BID_PRICE});
   book.OrderDelete(ItchOrderDelete{1, 1, 0});
   EXPECT_EQ(book.BidCount(), 0);
   EXPECT_EQ(book.BestBid(), 0);
}

TEST(OrderBook, add_cancel) {
   std::vector<Order> orders = {Order{}};
   OrderBook book{orders, {BID_PRICE}, {}};
   book.OrderAdd(ItchOrderAdd{1, 1, 0, 1, 2, BID_PRICE});
   book.OrderCancel(ItchOrderCancel{1, 1, 0, 1});
   EXPECT_EQ(book.BidCount(), 1);
   const auto order = book.OrderFromId(0);
   EXPECT_EQ(order->quantity, 1);
}

TEST(OrderBook, add_replace) {
   std::vector<Order> orders = {Order{}, Order{}};
   auto new_price = BID_PRICE + 1;
   OrderBook book{orders, {BID_PRICE, new_price}, {}};
   book.OrderAdd(ItchOrderAdd{1, 1, 0, 1, 1, BID_PRICE});
   book.OrderReplace(ItchOrderReplace{1, 1, 0, 1, 1, new_price});
   EXPECT_EQ(book.BidCount(), 1);
   EXPECT_EQ(book.BestBid(), new_price);
}

} // order_book::itch::v02
