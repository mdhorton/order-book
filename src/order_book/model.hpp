#pragma once

#include <cstdint>

namespace nostromo::order_book {
constexpr int32_t MAX_PRICE_LEVELS = 10'000;
constexpr int32_t MAX_ORDERS = 100'000;

// 24 bytes
struct PriceLevel {
   uint32_t price;
   uint32_t head_order_idx;
   uint32_t tail_order_idx;
   uint32_t quantity;
   uint32_t order_count;
   uint32_t bid;
};

// 32 bytes
struct Order {
   uint64_t timestamp;
   uint32_t order_id;
   uint32_t quantity;
   uint32_t next_idx;
   uint32_t prev_idx;
   uint32_t price_idx;
   uint32_t bid;
};

struct NewOrder {
   int32_t price;
   uint32_t order_id;
   uint32_t timestamp;
   uint32_t quantity;
   bool bid;
};

struct CancelOrder {
   uint32_t order_id;
};

// 23 bytes
struct ItchOrderAdd {
   uint16_t stock_code;
   uint64_t timestamp;
   uint32_t order_id;
   uint8_t bid;
   uint32_t quantity;
   uint32_t price;
} __attribute__((packed));

// 18 bytes
struct ItchOrderExecuted {
   uint16_t stock_code;
   uint64_t timestamp;
   uint32_t order_id;
   uint32_t quantity;
} __attribute__((packed));

// 23 bytes
struct ItchOrderExecutedPrice {
   uint16_t stock_code;
   uint64_t timestamp;
   uint32_t order_id;
   uint32_t quantity;
   uint8_t printable;
   uint32_t price;
} __attribute__((packed));

// 18 bytes
struct ItchOrderCancel {
   uint16_t stock_code;
   uint64_t timestamp;
   uint32_t order_id;
   uint32_t quantity;
} __attribute__((packed));

// 14 bytes
struct ItchOrderDelete {
   uint16_t stock_code;
   uint64_t timestamp;
   uint32_t order_id;
} __attribute__((packed));

// 26 bytes
struct ItchOrderReplace {
   uint16_t stock_code;
   uint64_t timestamp;
   uint32_t orig_order_id;
   uint32_t new_order_id;
   uint32_t quantity;
   uint32_t price;
} __attribute__((packed));
} // nostromo::order_book
