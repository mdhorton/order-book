#ifndef ORDER_BOOK_ITCH_v02_MODEL_HPP
#define ORDER_BOOK_ITCH_v02_MODEL_HPP

#include <cstdint>

namespace order_book::itch::v02 {

// 32 bytes
struct Order {
   uint64_t timestamp;
   uint32_t order_id;
   uint32_t quantity;
   uint32_t next_idx;
   uint32_t prev_idx;
   uint32_t price_idx;
   uint16_t bid;
};

// 16 bytes
struct PriceLevel {
   uint32_t price;
   uint32_t quantity;
   uint16_t order_count;
   uint16_t head_order_idx;
   uint16_t tail_order_idx;
   uint16_t idx;
};

} // order_book::itch::v02

#endif //ORDER_BOOK_ITCH_v02_MODEL_HPP
