#ifndef ORDER_BOOK_ITCH_ITCH_HPP
#define ORDER_BOOK_ITCH_ITCH_HPP

#include <cstdint>

#ifndef ITCH_PACKED
   #define ITCH_PACKED __attribute__((packed))
#endif

namespace order_book::itch {

constexpr uint32_t MAX_PRICE = 2'000'000'000;

struct ItchBase {
   uint64_t timestamp;
   uint32_t order_id;
   uint16_t stock_code;
} ITCH_PACKED;

struct ItchOrderAdd :
      ItchBase {
   uint8_t bid;
   uint32_t quantity;
   uint32_t price;
} ITCH_PACKED;

struct ItchOrderAddIdx :
      ItchOrderAdd {
   uint16_t price_idx;
} ITCH_PACKED;

struct ItchOrderExecuted :
      ItchBase {
   uint32_t quantity;
} ITCH_PACKED;

struct ItchOrderCancel :
      ItchBase {
   uint32_t quantity;
} ITCH_PACKED;

struct ItchOrderDelete :
      ItchBase {
} ITCH_PACKED;

struct ItchOrderReplace :
      ItchBase {
   uint32_t new_order_id;
   uint32_t quantity;
   uint32_t price;
} ITCH_PACKED;

struct ItchOrderReplaceIdx :
      ItchOrderReplace {
   uint16_t price_idx;
} ITCH_PACKED;

} //namespace order_book::itch

#endif //ORDER_BOOK_ITCH_ITCH_HPP
