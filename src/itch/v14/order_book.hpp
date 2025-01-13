#ifndef ORDER_BOOK_ITCH_V14_ORDER_BOOK_HPP
#define ORDER_BOOK_ITCH_V14_ORDER_BOOK_HPP

#include "itch/itch.hpp"

#include "nostromo/mmap.hpp"

#include <cstdint>
#include <cassert>
#include <vector>
#include <span>

// use uint32_t for order->bid.
namespace order_book::itch::v14 {

struct PriceLevel {
   uint32_t quantity;
};

struct Order {
   PriceLevel *level;
   uint32_t quantity;
   uint32_t bid;
};

class OrderBook {
   const std::span<Order> orders_;
   const std::span<PriceLevel> price_levels_[2];

   ItchOrderAddIdx tmp_order_add_{};
   ItchOrderDelete tmp_order_delete_{};

public:
   OrderBook(
         const std::span<Order> orders,
         const std::span<PriceLevel> bid_levels,
         const std::span<PriceLevel> ask_levels,
         const std::vector<uint32_t> &bid_prices,
         const std::vector<uint32_t> &ask_prices)
         : orders_{orders},
           price_levels_{bid_levels, ask_levels} {
      InitializePrices(bid_prices, price_levels_[1]);
      InitializePrices(ask_prices, price_levels_[0]);
   }

   static void InitializePrices(
         const auto &prices,
         auto &price_levels) {
      for (uint32_t idx = 0u; idx < prices.size(); ++idx) {
         const auto addr = &price_levels[idx];
         new(addr) PriceLevel;
      }
   }

   ALWAYS_INLINE
   void OrderAdd(const ItchOrderAddIdx &itch_order) const {
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
   PriceLevel *PriceLevelFromPrice(const ItchOrderAddIdx &order) const {
      return &price_levels_[order.bid][order.price_idx];
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
   void OrderAddImpl(const ItchOrderAddIdx &itch_order) const {
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
      price_level->quantity -= order->quantity;
   }
};

class OrderBooks {
   const nostromo::Mmap<Order> orders_mmap_;
   const nostromo::Mmap<OrderBook> order_books_mmap_;
   const nostromo::Mmap<PriceLevel> price_levels_mmap_;
   const std::span<Order> orders_;
   const std::span<OrderBook> order_books_;
   const std::span<PriceLevel> price_levels_;

public:
   OrderBooks(
         const auto max_order_id,
         const auto max_stock_code,
         const auto &stock_prices,
         const size_t page_size = 0u)
         : orders_mmap_{max_order_id + 1u, page_size},
           order_books_mmap_{max_stock_code + 1u, page_size},
           price_levels_mmap_{CountPriceLevels(stock_prices), page_size},
           orders_{orders_mmap_.Span()},
           order_books_{order_books_mmap_.Span()},
           price_levels_{price_levels_mmap_.Span()} {
      auto offset = 0u;
      for (auto &[stock_code, pair]: stock_prices) {
         const auto bid_prices = pair.first;
         const auto ask_prices = pair.second;
         const auto bid_cnt = bid_prices.size();
         const auto ask_cnt = ask_prices.size();
         const auto bid_levels = price_levels_.subspan(offset, bid_cnt);
         offset += bid_cnt;
         const auto ask_levels = price_levels_.subspan(offset, ask_cnt);
         offset += ask_cnt;
         const auto addr = &order_books_[stock_code];
         new(addr) OrderBook{orders_, bid_levels, ask_levels, bid_prices, ask_prices};
      }
   }

   ALWAYS_INLINE
   void OrderAdd(const ItchOrderAddIdx &order) const {
      auto &order_book = order_books_[order.stock_code];
      order_book.OrderAdd(order);
   }

   ALWAYS_INLINE
   void OrderExecuted(const ItchOrderExecuted &order) const {
      const auto &order_book = order_books_[order.stock_code];
      order_book.OrderExecuted(order);
   }

   ALWAYS_INLINE
   void OrderCancel(const ItchOrderCancel &order) const {
      const auto &order_book = order_books_[order.stock_code];
      order_book.OrderCancel(order);
   }

   ALWAYS_INLINE
   void OrderDelete(const ItchOrderDelete &order) const {
      const auto &order_book = order_books_[order.stock_code];
      order_book.OrderDelete(order);
   }

   ALWAYS_INLINE
   void OrderReplace(const ItchOrderReplaceIdx &order) const {
      auto &order_book = order_books_[order.stock_code];
      order_book.OrderReplace(order);
   }

private:
   auto CountPriceLevels(const auto &stock_prices) const {
      auto cnt = 0u;
      for (const auto &[stock_code, pair]: stock_prices) {
         cnt += pair.first.size() + pair.second.size();
      }
      return cnt;
   }
};

} // order_book::itch::v14

#endif //ORDER_BOOK_ITCH_V14_ORDER_BOOK_HPP
