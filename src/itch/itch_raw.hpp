#ifndef ORDER_BOOK_ITCH_ITCH_RAW_HPP
#define ORDER_BOOK_ITCH_ITCH_RAW_HPP

#include <cstdint>

namespace order_book::itch {

struct ItchRawBase {
   uint16_t stock_code;
   uint64_t tracking_num: 2,
         timestamp: 6;
   uint64_t order_id;
} __attribute__((packed));

struct ItchRawOrderAdd :
      ItchRawBase {
   char bid;
   uint32_t quantity;
   uint64_t stock;
   uint32_t price;
} __attribute__((packed));

struct ItchRawOrderAddMpid :
      ItchRawOrderAdd {
   uint32_t attribution;
} __attribute__((packed));

struct ItchRawOrderExecuted :
      ItchRawBase {
   uint32_t quantity;
   uint64_t match_num;
} __attribute__((packed));

struct ItchRawOrderExecutedPrice :
      ItchRawOrderExecuted {
   char printable;
   uint32_t price;
} __attribute__((packed));

struct ItchRwOrderCancel :
      ItchRawBase {
   uint32_t quantity;
} __attribute__((packed));

struct ItchRawOrderdelete :
      ItchRawBase {
} __attribute__((packed));

} //namespace order_book::itch

#endif //ORDER_BOOK_ITCH_ITCH_RAW_HPP
