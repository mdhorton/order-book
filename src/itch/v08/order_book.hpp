#ifndef ORDER_BOOK_ITCH_V08_ORDER_BOOK_HPP
#define ORDER_BOOK_ITCH_V08_ORDER_BOOK_HPP

#include "itch/defs.hpp"
#include "itch/common.hpp"
#include "itch/usings.hpp"
#include "itch/itch.hpp"

#include "nostromo/mmap.hpp"

#include <boost/unordered/unordered_flat_map.hpp>

#include <cstdint>
#include <cassert>
#include <vector>
#include <span>

// use std::span instead of std::vector for price levels.
namespace order_book::itch::v08 {

struct PriceLevel {
   uint32_t quantity;
};

struct Order {
   PriceLevel *level;
   uint32_t quantity;
   uint8_t bid;
};

using PRICE_MAP = boost::unordered_flat_map<uint32_t, PriceLevel *>;

class OrderBook {
   const std::span<Order> orders_;
   const std::span<PriceLevel> ask_levels_;
   const std::span<PriceLevel> bid_levels_;

   PRICE_MAP ask_price_map_{};
   PRICE_MAP bid_price_map_{};
   ItchOrderAdd tmp_order_add_{};
   ItchOrderDelete tmp_order_delete_{};

public:
   OrderBook(
         const std::span<Order> orders,
         const std::span<PriceLevel> ask_levels,
         const std::span<PriceLevel> bid_levels,
         const std::vector<uint32_t> &asks,
         const std::vector<uint32_t> &bids)
         : orders_{orders},
           ask_levels_{ask_levels},
           bid_levels_{bid_levels} {
      InitializePrices(asks, ask_levels_, ask_price_map_);
      InitializePrices(bids, bid_levels_, bid_price_map_);
   }

   static void InitializePrices(
         const auto &prices,
         const auto levels,
         auto &price_map) {
      for (uint32_t idx = 0u; idx < prices.size(); ++idx) {
         const auto addr = &levels[idx];
         new(addr) PriceLevel;
         price_map[prices[idx]] = addr;
      }
   }

   ALWAYS_INLINE
   void OrderAdd(const ItchOrderAdd &itch_order) {
      OrderAddImpl(itch_order);
   }

   ALWAYS_INLINE
   void OrderExecuted(const ItchOrderExecuted &itch_order) const {
      const auto order = OrderFromId(itch_order.order_id);
      const auto level = PriceLevelFromIndex(*order);
      assert(order->quantity >= itch_order.quantity);
      assert(level->quantity >= itch_order.quantity);
      order->quantity -= itch_order.quantity;
      level->quantity -= itch_order.quantity;
   }

   ALWAYS_INLINE
   void OrderCancel(const ItchOrderCancel &itch_order) const {
      const auto order = OrderFromId(itch_order.order_id);
      const auto level = PriceLevelFromIndex(*order);
      assert(order->quantity > itch_order.quantity);
      assert(level->quantity > itch_order.quantity);
      order->quantity -= itch_order.quantity;
      level->quantity -= itch_order.quantity;
   }

   ALWAYS_INLINE
   void OrderDelete(const ItchOrderDelete &itch_order) const {
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
      return order.bid ? bid_price_map_[order.price] : ask_price_map_[order.price];
   }

   [[nodiscard]] ALWAYS_INLINE static
   PriceLevel *PriceLevelFromIndex(const Order &order) {
      return order.level;
   }

   [[nodiscard]] ALWAYS_INLINE
   Order *OrderFromId(const uint32_t order_id) const {
      return &orders_[order_id];
   }

private:
   ALWAYS_INLINE
   void OrderAddImpl(const ItchOrderAdd &itch_order) {
      const auto level = PriceLevelFromPrice(itch_order);
      const auto order = OrderFromId(itch_order.order_id);

      order->level = level;
      order->quantity = itch_order.quantity;
      order->bid = itch_order.bid;

      level->quantity += itch_order.quantity;
   }

   ALWAYS_INLINE
   void OrderDeleteImpl(const ItchOrderDelete &itch_order) const {
      const auto order = OrderFromId(itch_order.order_id);
      const auto level = PriceLevelFromIndex(*order);
      assert(level->quantity >= order->quantity);
      order->quantity = 0u;
      level->quantity -= order->quantity;
   }
};

class OrderBooks {
   const nostromo::Mmap<Order> orders_mmap_;
   const nostromo::Mmap<OrderBook> order_books_mmap_;
   const nostromo::Mmap<PriceLevel> levels_mmap_;
   const std::span<Order> orders_;
   const std::span<OrderBook> order_books_;
   const std::span<PriceLevel> levels_;

public:
   OrderBooks(
         const auto max_order_id,
         const auto max_stock_code,
         const STOCK_PRICE_MAP &stock_price_map,
         const size_t orders_page_size,
         const size_t other_page_size)
         : orders_mmap_{max_order_id + 1u, orders_page_size},
           order_books_mmap_{max_stock_code + 1u, other_page_size},
           levels_mmap_{CountPriceLevels(stock_price_map), other_page_size},
           orders_{orders_mmap_.Span()},
           order_books_{order_books_mmap_.Span()},
           levels_{levels_mmap_.Span()} {
      auto offset = 0u;
      for (const auto &[stock_code, pair]: stock_price_map) {
         const auto &[asks, bids] = pair;
         const auto ask_cnt = asks.size();
         const auto bid_cnt = bids.size();
         const auto ask_levels = levels_.subspan(offset, ask_cnt);
         offset += ask_cnt;
         const auto bid_levels = levels_.subspan(offset, bid_cnt);
         offset += bid_cnt;
         const auto addr = &order_books_[stock_code];
         new(addr) OrderBook{orders_, ask_levels, bid_levels, asks, bids};
      }
   }

   INLINE
   void OrderAdd(const ItchOrderAdd &order) const {
      auto &order_book = order_books_[order.stock_code];
      order_book.OrderAdd(order);
   }

   INLINE
   void OrderExecuted(const ItchOrderExecuted &order) const {
      const auto &order_book = order_books_[order.stock_code];
      order_book.OrderExecuted(order);
   }

   INLINE
   void OrderCancel(const ItchOrderCancel &order) const {
      const auto &order_book = order_books_[order.stock_code];
      order_book.OrderCancel(order);
   }

   INLINE
   void OrderDelete(const ItchOrderDelete &order) const {
      const auto &order_book = order_books_[order.stock_code];
      order_book.OrderDelete(order);
   }

   INLINE
   void OrderReplace(const ItchOrderReplace &order) const {
      auto &order_book = order_books_[order.stock_code];
      order_book.OrderReplace(order);
   }

private:
   [[nodiscard]]
   static size_t CountPriceLevels(const STOCK_PRICE_MAP &stock_price_map) {
      size_t total_count = 0u;
      for (const auto &[stock_code, pair]: stock_price_map) {
         const auto &[asks, bids] = pair;
         const auto cnt = asks.size() + bids.size();
         assert(cnt > 0);
         total_count += cnt;
      }
      return total_count;
   }
};

} // order_book::itch::v08

#endif //ORDER_BOOK_ITCH_V08_ORDER_BOOK_HPP
