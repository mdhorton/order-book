#include <gtest/gtest.h>

#include "order_book_1/order_book.hpp"

namespace nostromo::order_book::order_book_1 {
TEST(OrderBook, bid_ask) {
   OrderBook book{100, 2500};
   book.HandleNewOrder(NewOrder{400000, 1, 1, 1, true});
   book.HandleNewOrder(NewOrder{500000, 2, 1, 1, false});
   EXPECT_EQ(book.BestBid(), 400000);
   EXPECT_EQ(book.BestAsk(), 500000);
}

TEST(OrderBook, bid_cancel) {
   OrderBook book{100, 2500};
   book.HandleNewOrder(NewOrder{400000, 1, 1, 1, true});
   book.HandleCancelOrder(CancelOrder{1});
   EXPECT_EQ(book.BestBid(), 250000);
}
} // nostromo::order_book::order_book_1
