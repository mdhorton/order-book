#pragma once

#include <cstdint>

namespace nostromo::order_book {
constexpr int32_t MAX_PRICE_LEVELS = 10'000;
constexpr int32_t MAX_ORDERS = 100'000;

// 16 bytes
struct PriceLevel {
   uint32_t head_order_idx;
   uint32_t tail_order_idx;
   uint32_t quantity;
   uint32_t order_count;
};

// 24 bytes
struct Order {
   uint32_t order_id;
   uint32_t timestamp;
   uint32_t quantity;
   uint32_t next_idx;
   uint32_t prev_idx;
   int32_t price_idx;
};

struct NewOrder {
   int32_t price;
   uint32_t order_id;
   uint32_t timestamp;
   uint32_t quantity;
   bool bid;
} __attribute__((packed));

struct CancelOrder {
   uint32_t order_id;
};

struct OrderAdd {
   uint16_t market_id;
   uint64_t timestamp;
   uint32_t order_id;
   bool bid;
   uint32_t quantity;
   uint32_t price;
} __attribute__((packed));

struct OrderExecuted {
   uint16_t market_id;
   uint64_t timestamp;
   uint32_t order_id;
   uint32_t quantity;
} __attribute__((packed));

struct OrderExecutedPrice {
   uint16_t market_id;
   uint64_t timestamp;
   uint32_t order_id;
   uint32_t quantity;
   bool printable;
   uint32_t price;
} __attribute__((packed));

struct OrderCancel {
   uint16_t market_id;
   uint64_t timestamp;
   uint32_t order_id;
   uint32_t quantity;
} __attribute__((packed));

struct OrderDelete {
   uint16_t market_id;
   uint64_t timestamp;
   uint32_t order_id;
} __attribute__((packed));

struct OrderReplace {
   uint16_t market_id;
   uint64_t timestamp;
   uint32_t orig_order_id;
   uint32_t new_order_id;
   uint32_t quantity;
   uint32_t price;
} __attribute__((packed));
} // nostromo::order_book
