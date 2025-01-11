#ifndef ORDER_BOOK_ITCH_V12_ORDER_BOOK_HPP
#define ORDER_BOOK_ITCH_V12_ORDER_BOOK_HPP

#include <cstdint>
#include <cassert>
#include <span>

#include <boost/unordered/unordered_flat_map.hpp>

#include "nostromo/mmap.hpp"

#include "itch/itch.hpp"

#include <immintrin.h>


namespace order_book::itch::v12 {
struct PriceLevel {
   uint32_t quantity;
   uint16_t price_idx;
   uint8_t simd;
};

struct Order {
   PriceLevel *level;
   uint32_t quantity;
   uint8_t bid;
};

using PRICE_MAP = boost::unordered_flat_map<uint32_t, PriceLevel *>;

class OrderBook {
   static constexpr int8_t LOOKUP_TABLE[] = {
      0, 0, 0, 0,
      1, 1, 1, 1,
      2, 2, 2, 2,
      3, 3, 3, 3,
      4, 4, 4, 4,
      5, 5, 5, 5,
      6, 6, 6, 6,
      7, 7, 7, 7
   };

   const std::span<Order> orders_;
   const std::span<int32_t> prices_[2];

   std::vector<PriceLevel> levels_[2] = {std::vector<PriceLevel>(), std::vector<PriceLevel>()};
   PRICE_MAP price_maps_[2] = {PRICE_MAP{}, PRICE_MAP{}};
   PriceLevel *prev_level_[2] = {nullptr, nullptr};
   ItchOrderAdd tmp_order_add_{};
   ItchOrderDelete tmp_order_delete_{};

   // asks=15,14,13  12,11,10=bids
public:
   OrderBook(
      const std::span<Order> orders,
      const std::span<int32_t> bid_prices,
      const std::span<int32_t> ask_prices)
      : orders_{orders},
        prices_{bid_prices, ask_prices} {
      InitializePrices(bid_prices, levels_[1], price_maps_[1]);
      InitializePrices(ask_prices, levels_[0], price_maps_[0]);
      if (!bid_prices.empty()) prev_level_[1] = &levels_[1][0];
      if (!ask_prices.empty()) prev_level_[0] = &levels_[0][0];
   }

   static void InitializePrices(
      const std::span<int32_t> prices,
      std::vector<PriceLevel> &price_levels,
      PRICE_MAP &price_map) {
      price_levels.reserve(prices.size());

      for (uint64_t idx = 0; idx < prices.size(); ++idx) {
         if (idx > 65'535) {
            throw std::runtime_error("price index out of range");
         }
         auto price_idx = static_cast<uint16_t>(idx);
         price_levels.emplace_back(0, price_idx, 0);
         price_map[prices[idx]] = &price_levels.back();
      }
   }

   ALWAYS_INLINE
   void OrderAdd(const ItchOrderAdd &itch_order) {
      OrderAddImpl(itch_order);
   }

   ALWAYS_INLINE
   void OrderExecuted(const ItchOrderExecuted &itch_order) {
      const auto order = OrderFromId(itch_order.order_id);
      const auto price_level = PriceLevelFromOrder(*order);
      assert(order->quantity >= itch_order.quantity);
      assert(price_level->quantity >= itch_order.quantity);
      order->quantity -= itch_order.quantity;
      price_level->quantity -= itch_order.quantity;
   }

   ALWAYS_INLINE
   void OrderCancel(const ItchOrderCancel &itch_order) {
      const auto order = OrderFromId(itch_order.order_id);
      const auto price_level = PriceLevelFromOrder(*order);
      assert(order->quantity > itch_order.quantity);
      assert(price_level->quantity > itch_order.quantity);
      order->quantity -= itch_order.quantity;
      price_level->quantity -= itch_order.quantity;
   }

   ALWAYS_INLINE
   void OrderDelete(const ItchOrderDelete &itch_order) {
      OrderDeleteImpl(itch_order);
   }

   ALWAYS_INLINE
   void OrderReplace(const ItchOrderReplace &itch_order) {
      tmp_order_delete_.stock_code = itch_order.stock_code;
      tmp_order_delete_.order_id = itch_order.order_id;
      OrderDeleteImpl(tmp_order_delete_);
      const auto orig_order = OrderFromId(itch_order.order_id);
      tmp_order_add_.stock_code = itch_order.stock_code;
      tmp_order_add_.order_id = itch_order.new_order_id;
      tmp_order_add_.bid = orig_order->bid;
      tmp_order_add_.quantity = itch_order.quantity;
      tmp_order_add_.price = itch_order.price;
      OrderAddImpl(tmp_order_add_);
   }

   [[nodiscard]] ALWAYS_INLINE
   PriceLevel *PriceLevelFromItchOrder(const ItchOrderAdd &order) {
      const auto bid = order.bid;
      const auto price = order.price;
      const auto level = prev_level_[bid];

      if (!level->simd) {
         prev_level_[bid] = price_maps_[bid][price];
         return prev_level_[bid];
      }

      const auto ptr = prices_[bid].data();
      const auto data = _mm256_load_si256(reinterpret_cast<__m256i *>(ptr));
      const auto test = _mm256_set1_epi32(static_cast<int32_t>(price));
      const auto cmp = _mm256_cmpeq_epi32(test, data);
      const auto mask = _mm256_movemask_epi8(cmp);

      if (mask == 0) {
         prev_level_[bid] = price_maps_[bid][price];
         return prev_level_[bid];
      }

      const auto tzcnt = __tzcnt_u32(mask);
      const auto idx = LOOKUP_TABLE[tzcnt] + level->price_idx;
      prev_level_[bid] = &levels_[bid][idx];
      return prev_level_[bid];
   }

   [[nodiscard]] ALWAYS_INLINE
   PriceLevel *PriceLevelFromOrder(const Order &order) {
      prev_level_[order.bid] = order.level;
      return order.level;
   }

   [[nodiscard]] ALWAYS_INLINE
   Order *OrderFromId(const uint32_t order_id) const {
      return &orders_[order_id];
   }

private:
   ALWAYS_INLINE
   void OrderAddImpl(const ItchOrderAdd &itch_order) {
      const auto price_level = PriceLevelFromItchOrder(itch_order);
      const auto order = OrderFromId(itch_order.order_id);

      order->level = price_level;
      order->quantity = itch_order.quantity;
      order->bid = itch_order.bid;

      price_level->quantity += itch_order.quantity;
   }

   ALWAYS_INLINE
   void OrderDeleteImpl(const ItchOrderDelete &itch_order) {
      const auto order = OrderFromId(itch_order.order_id);
      const auto price_level = PriceLevelFromOrder(*order);
      assert(price_level->quantity >= order->quantity);
      price_level->quantity -= order->quantity;
   }
};

class OrderBooks {
   const nostromo::Mmap<Order> orders_mmap_;
   const nostromo::Mmap<OrderBook> order_books_mmap_;
   const std::span<Order> orders_;
   const std::span<OrderBook> order_books_;

public:
   OrderBooks(
      auto max_order_id,
      auto max_stock_code,
      auto &stock_prices,
      const size_t page_size = 0)
      : orders_mmap_{max_order_id + 1u, page_size},
        order_books_mmap_{max_stock_code + 1u, page_size},
        orders_{orders_mmap_.Span()},
        order_books_{order_books_mmap_.Span()} {
      for (auto &[stock_code, pair]: stock_prices) {
         const auto addr = &order_books_[stock_code];
         // std::span<int32_t> b = (std::vector<int32_t>&) pair.first;
         // const std::vector<int32_t> a = pair.first;
         // auto foo = std::span<int32_t>(b);
         // std::span<int32_t> bids = std::span<int32_t>(b.data(), b.end(), b.size());
         // std::span<int32_t> asks = std::span<int32_t>(a.data(), a.size());
         new(addr) OrderBook(orders_, b, pair.second);
      }
   }

   ALWAYS_INLINE
   void OrderAdd(const ItchOrderAdd &order) const {
      auto &order_book = order_books_[order.stock_code];
      order_book.OrderAdd(order);
   }

   ALWAYS_INLINE
   void OrderExecuted(const ItchOrderExecuted &order) const {
      auto &order_book = order_books_[order.stock_code];
      order_book.OrderExecuted(order);
   }

   ALWAYS_INLINE
   void OrderCancel(const ItchOrderCancel &order) const {
      auto &order_book = order_books_[order.stock_code];
      order_book.OrderCancel(order);
   }

   ALWAYS_INLINE
   void OrderDelete(const ItchOrderDelete &order) const {
      auto &order_book = order_books_[order.stock_code];
      order_book.OrderDelete(order);
   }

   ALWAYS_INLINE
   void OrderReplace(const ItchOrderReplace &order) const {
      auto &order_book = order_books_[order.stock_code];
      order_book.OrderReplace(order);
   }
};
} // order_book::itch::v12

#endif //ORDER_BOOK_ITCH_V12_ORDER_BOOK_HPP
