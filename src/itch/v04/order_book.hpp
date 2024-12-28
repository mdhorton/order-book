#ifndef ORDER_BOOK_ITCH_V04_ORDER_BOOK_HPP
#define ORDER_BOOK_ITCH_V04_ORDER_BOOK_HPP

#include <iostream>
#include <array>

#include "itch/prices/prices.hpp"

namespace order_book::itch::v04 {

class Foo {
public:
   static void Bar() {
      static_assert(prices::lookup[0][1].first == 2u);
   }
};

} // order_book::itch::v04

#endif //ORDER_BOOK_ITCH_V04_ORDER_BOOK_HPP
