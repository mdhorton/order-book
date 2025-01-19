#ifndef ORDER_BOOK_ITCH_V12_ORDER_BOOK_HPP
#define ORDER_BOOK_ITCH_V12_ORDER_BOOK_HPP

#include "itch/defs.hpp"
#include "itch/common.hpp"
#include "itch/usings.hpp"
#include "itch/itch.hpp"

#include "nostromo/mmap.hpp"

#include <cstdint>
#include <cassert>
#include <vector>
#include <span>

// removed price maps, use price idx.
namespace order_book::itch::v12 {

struct PriceLevel {
   uint32_t quantity;
};

struct Order {
   PriceLevel *level;
   uint32_t quantity;
   uint8_t bid;
};

class OrderBook {
   const std::span<Order> orders_;

   std::vector<PriceLevel> levels_[2]{};
   ItchOrderAddIdx tmp_order_add_{};
   ItchOrderDelete tmp_order_delete_{};

public:
   OrderBook(
         const std::span<Order> orders,
         const std::vector<uint32_t> &asks,
         const std::vector<uint32_t> &bids)
         : orders_{orders} {
      InitializePrices(asks, &levels_[0]);
      InitializePrices(bids, &levels_[1]);
   }

   static void InitializePrices(
         const auto &prices,
         auto levels) {
      levels->reserve(prices.size());

      for (auto idx = 0u; idx < prices.size(); ++idx) {
         levels->push_back(PriceLevel{});
      }
   }

   ALWAYS_INLINE
   void OrderAdd(const ItchOrderAddIdx &itch_order) {
      OrderAddImpl(itch_order);
   }

   ALWAYS_INLINE
   void OrderExecuted(const ItchOrderExecuted &itch_order) const {
      const auto order = OrderFromId(itch_order.order_id);
      const auto price_level = PriceLevelFromIndex(*order);
      assert(order->quantity >= itch_order.quantity);
      assert(price_level->quantity >= itch_order.quantity);
      order->quantity -= itch_order.quantity;
      price_level->quantity -= itch_order.quantity;
   }

   ALWAYS_INLINE
   void OrderCancel(const ItchOrderCancel &itch_order) const {
      const auto order = OrderFromId(itch_order.order_id);
      const auto price_level = PriceLevelFromIndex(*order);
      assert(order->quantity > itch_order.quantity);
      assert(price_level->quantity > itch_order.quantity);
      order->quantity -= itch_order.quantity;
      price_level->quantity -= itch_order.quantity;
   }

   ALWAYS_INLINE
   void OrderDelete(const ItchOrderDelete &itch_order) const {
      OrderDeleteImpl(itch_order);
   }

   ALWAYS_INLINE
   void OrderReplace(const ItchOrderReplaceIdx &itch_order) {
      tmp_order_delete_.stock_code = itch_order.stock_code;
      tmp_order_delete_.order_id = itch_order.order_id;
      OrderDeleteImpl(tmp_order_delete_);
      const auto orig_order = OrderFromId(itch_order.order_id);
      tmp_order_add_.stock_code = itch_order.stock_code;
      tmp_order_add_.order_id = itch_order.new_order_id;
      tmp_order_add_.bid = orig_order->bid;
      tmp_order_add_.quantity = itch_order.quantity;
      tmp_order_add_.price = itch_order.price;
      tmp_order_add_.price_idx = itch_order.price_idx;
      OrderAddImpl(tmp_order_add_);
   }

   [[nodiscard]] ALWAYS_INLINE
   PriceLevel *PriceLevelFromPrice(const ItchOrderAddIdx &order) {
      return &levels_[order.bid][order.price_idx];
   }

   [[nodiscard]] ALWAYS_INLINE
   static PriceLevel *PriceLevelFromIndex(const Order &order) {
      return order.level;
   }

   [[nodiscard]] ALWAYS_INLINE
   Order *OrderFromId(const uint32_t order_id) const {
      return &orders_[order_id];
   }

private:
   ALWAYS_INLINE
   void OrderAddImpl(const ItchOrderAddIdx &itch_order) {
      const auto price_level = PriceLevelFromPrice(itch_order);
      const auto order = OrderFromId(itch_order.order_id);

      order->level = price_level;
      order->quantity = itch_order.quantity;
      order->bid = itch_order.bid;

      price_level->quantity += itch_order.quantity;
   }

   ALWAYS_INLINE
   void OrderDeleteImpl(const ItchOrderDelete &itch_order) const {
      const auto order = OrderFromId(itch_order.order_id);
      const auto price_level = PriceLevelFromIndex(*order);
      assert(price_level->quantity >= order->quantity);
      order->quantity = 0u;
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
         const auto max_order_id,
         const auto max_stock_code,
         const STOCK_PRICE_MAP &stock_price_map,
         const size_t orders_page_size,
         const size_t other_page_size)
         : orders_mmap_{max_order_id + 1u, orders_page_size},
           order_books_mmap_{max_stock_code + 1u, other_page_size},
           orders_{orders_mmap_.Span()},
           order_books_{order_books_mmap_.Span()} {
      for (const auto &[stock_code, pair]: stock_price_map) {
         const auto &[asks, bids] = pair;
         const auto addr = &order_books_[stock_code];
         new(addr) OrderBook{orders_, asks, bids};
      }
   }

   INLINE
   void OrderAdd(const ItchOrderAddIdx &order) const {
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
   void OrderReplace(const ItchOrderReplaceIdx &order) const {
      auto &order_book = order_books_[order.stock_code];
      order_book.OrderReplace(order);
   }
};

} // order_book::itch::v12

#endif //ORDER_BOOK_ITCH_V12_ORDER_BOOK_HPP
