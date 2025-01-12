#ifndef ORDER_BOOK_ITCH_ITCH_RAW_HPP
#define ORDER_BOOK_ITCH_ITCH_RAW_HPP

#include "common.hpp"

#include <linux/types.h>

namespace order_book::itch {

struct ItchRawBase {
   __be16 stock_code;
   __be16 tracking_num;
   __u8 timestamp[6];
   __be64 order_id;
} PACKED;


struct ItchRawOrderAdd :
      ItchRawBase {
   __u8 bid;
   __be32 quantity;
   __u8 stock[8];
   __be32 price;
} PACKED;

struct ItchRawOrderAddMpid :
      ItchRawOrderAdd {
   __be32 attribution;
} PACKED;

struct ItchRawOrderExecuted :
      ItchRawBase {
   __be32 quantity;
   __be64 match_num;
} PACKED;

struct ItchRawOrderExecutedPrice :
      ItchRawOrderExecuted {
   __u8 printable;
   __be32 price;
} PACKED;

struct ItchRawOrderCancel :
      ItchRawBase {
   __be32 quantity;
} PACKED;

struct ItchRawOrderDelete :
      ItchRawBase {
} PACKED;

struct ItchRawOrderReplace :
      ItchRawBase {
   __be64 new_order_id;
   __be32 quantity;
   __be32 price;
} PACKED;

} //namespace order_book::itch

#endif //ORDER_BOOK_ITCH_ITCH_RAW_HPP
