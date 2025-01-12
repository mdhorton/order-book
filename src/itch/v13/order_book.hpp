#ifndef ORDER_BOOK_ITCH_V13_ORDER_BOOK_HPP
#define ORDER_BOOK_ITCH_V13_ORDER_BOOK_HPP

#include <cstdint>
#include <cassert>
#include <span>

#include <boost/unordered/unordered_flat_map.hpp>

#include "nostromo/mmap.hpp"

#include "itch/itch.hpp"

namespace order_book::itch::v13 {
struct alignas(8) PriceLevel {
   uint32_t quantity;
   int32_t idx;
};

struct alignas(16) Order {
   PriceLevel *level;
   uint32_t quantity;
   uint8_t bid;
};

using PRICE_MAP = boost::unordered_flat_map<uint32_t, PriceLevel *>;

class OrderBook {
   const std::span<Order> orders_;

   std::vector<PriceLevel> bid_levels_{};
   std::vector<PriceLevel> ask_levels_{};
   PRICE_MAP price_maps_[2] = {PRICE_MAP{}, PRICE_MAP{}};
   ItchOrderAdd tmp_order_add_{};
   ItchOrderDelete tmp_order_delete_{};
   int32_t prev_level_idx[2] = {0, 0};

public:
   OrderBook(
         const std::span<Order> orders,
         const std::vector<uint32_t> &bid_prices,
         const std::vector<uint32_t> &ask_prices)
         : orders_{orders} {
      InitializePrices(bid_prices, bid_levels_, price_maps_[1]);
      InitializePrices(ask_prices, ask_levels_, price_maps_[0]);
   }

   static void InitializePrices(
         const std::vector<uint32_t> &prices,
         std::vector<PriceLevel> &price_levels,
         PRICE_MAP &price_map) {
      price_levels.reserve(prices.size());

      for (uint32_t idx = 0; idx < prices.size(); ++idx) {
         price_levels.emplace_back(0, idx);
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
      const auto price_level = PriceLevelFromIndex(*order);
      assert(order->quantity >= itch_order.quantity);
      assert(price_level->quantity >= itch_order.quantity);
      order->quantity -= itch_order.quantity;
      price_level->quantity -= itch_order.quantity;
   }

   ALWAYS_INLINE
   void OrderCancel(const ItchOrderCancel &itch_order) {
      const auto order = OrderFromId(itch_order.order_id);
      const auto price_level = PriceLevelFromIndex(*order);
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
   PriceLevel *PriceLevelFromPrice(const ItchOrderAdd &order) {
      return price_maps_[order.bid][order.price];
   }

   [[nodiscard]] ALWAYS_INLINE
   PriceLevel *PriceLevelFromIndex(const Order &order) {
      prev_level_idx[order.bid] = order.level->idx;
      return order.level;
   }

   [[nodiscard]] ALWAYS_INLINE
   Order *OrderFromId(const uint32_t order_id) const {
      return &orders_[order_id];
   }

private:
   ALWAYS_INLINE
   void OrderAddImpl(const ItchOrderAdd &itch_order) {
      const auto price_level = PriceLevelFromPrice(itch_order);
      const auto order = OrderFromId(itch_order.order_id);

      const auto diff = price_level->idx - ;

      order->level = price_level;
      order->quantity = itch_order.quantity;
      order->bid = itch_order.bid;

      price_level->quantity += itch_order.quantity;
   }

   ALWAYS_INLINE
   void OrderDeleteImpl(const ItchOrderDelete &itch_order) {
      const auto order = OrderFromId(itch_order.order_id);
      const auto price_level = PriceLevelFromIndex(*order);
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
         auto addr = &order_books_[stock_code];
         new(addr) OrderBook(orders_, pair.first, pair.second);
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
} // order_book::itch::v13

#endif //ORDER_BOOK_ITCH_V13_ORDER_BOOK_HPP
