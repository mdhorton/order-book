#ifndef ORDER_BOOK_ITCH_ITCH_RAW_HPP
#define ORDER_BOOK_ITCH_ITCH_RAW_HPP

#include <linux/types.h>

#ifndef ITCH_PACKED
   #define ITCH_PACKED __attribute__((packed))
#endif

namespace order_book::itch {

struct ItchRawBase {
   __be16 stock_code;
   __be16 tracking_num;
   __u8 timestamp[6];
   __be64 order_id;
} ITCH_PACKED;


struct ItchRawOrderAdd :
      ItchRawBase {
   __u8 bid;
   __be32 quantity;
   __u8 stock[8];
   __be32 price;
} ITCH_PACKED;

struct ItchRawOrderAddMpid :
      ItchRawOrderAdd {
   __be32 attribution;
} ITCH_PACKED;

struct ItchRawOrderExecuted :
      ItchRawBase {
   __be32 quantity;
   __be64 match_num;
} ITCH_PACKED;

struct ItchRawOrderExecutedPrice :
      ItchRawOrderExecuted {
   __u8 printable;
   __be32 price;
} ITCH_PACKED;

struct ItchRawOrderCancel :
      ItchRawBase {
   __be32 quantity;
} ITCH_PACKED;

struct ItchRawOrderDelete :
      ItchRawBase {
} ITCH_PACKED;

struct ItchRawOrderReplace :
      ItchRawBase {
   __be64 new_order_id;
   __be32 quantity;
   __be32 price;
} ITCH_PACKED;

} //namespace order_book::itch

#endif //ORDER_BOOK_ITCH_ITCH_RAW_HPP
