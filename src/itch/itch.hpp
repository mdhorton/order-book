#ifndef ORDER_BOOK_ITCH_ITCH_HPP
#define ORDER_BOOK_ITCH_ITCH_HPP

#include <cstdint>

#include "common.hpp"

namespace order_book::itch {

constexpr uint32_t MAX_PRICE = 2'000'000'000;

// 23 bytes
struct ItchOrderAdd {
   uint16_t stock_code;
   uint64_t timestamp;
   uint32_t order_id;
   uint8_t bid;
   uint32_t quantity;
   uint32_t price;
} PACKED;

// 18 bytes
struct ItchOrderExecuted {
   uint16_t stock_code;
   uint64_t timestamp;
   uint32_t order_id;
   uint32_t quantity;
} PACKED;

// 18 bytes
struct ItchOrderCancel {
   uint16_t stock_code;
   uint64_t timestamp;
   uint32_t order_id;
   uint32_t quantity;
} PACKED;

// 14 bytes
struct ItchOrderDelete {
   uint16_t stock_code;
   uint64_t timestamp;
   uint32_t order_id;
} PACKED;

// 26 bytes
struct ItchOrderReplace {
   uint16_t stock_code;
   uint64_t timestamp;
   uint32_t orig_order_id;
   uint32_t new_order_id;
   uint32_t quantity;
   uint32_t price;
} PACKED;

} //namespace order_book::itch

#endif //ORDER_BOOK_ITCH_ITCH_HPP
