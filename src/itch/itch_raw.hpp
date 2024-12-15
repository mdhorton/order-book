#ifndef ORDER_BOOK_ITCH_ITCH_RAW_HPP
#define ORDER_BOOK_ITCH_ITCH_RAW_HPP

#include <linux/types.h>

namespace order_book::itch {

struct ItchRawBase {
   __be16 stock_code;
   __be16 tracking_num;
   __u8 timestamp[6];
   __be64 order_id;
} __attribute__((packed));

struct ItchRawOrderAdd :
      ItchRawBase {
   __u8 bid;
   __be32 quantity;
   __u8 stock[8];
   __be32 price;
} __attribute__((packed));

struct ItchRawOrderAddMpid :
      ItchRawOrderAdd {
   __be32 attribution;
} __attribute__((packed));

struct ItchRawOrderExecuted :
      ItchRawBase {
   __be32 quantity;
   __be64 match_num;
} __attribute__((packed));

struct ItchRawOrderExecutedPrice :
      ItchRawOrderExecuted {
   __u8 printable;
   __be32 price;
} __attribute__((packed));

struct ItchRawOrderCancel :
      ItchRawBase {
   __be32 quantity;
} __attribute__((packed));

struct ItchRawOrderDelete :
      ItchRawBase {
} __attribute__((packed));

struct ItchRawOrderReplace :
      ItchRawBase {
   __be64 new_order_id;
   __be32 quantity;
   __be32 price;
} __attribute__((packed));

} //namespace order_book::itch

#endif //ORDER_BOOK_ITCH_ITCH_RAW_HPP
