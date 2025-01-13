#ifndef ORDER_BOOK_ITCH_V06_ORDER_BOOK_HPP
#define ORDER_BOOK_ITCH_V06_ORDER_BOOK_HPP

#include "itch/itch.hpp"

#include "nostromo/mmap.hpp"

#include <boost/unordered/unordered_flat_map.hpp>

#include <cstdint>
#include <cassert>
#include <vector>
#include <span>

// removed order->bid and set it as a bit on order->price_idx.
namespace order_book::itch::v06 {

struct PriceLevel {
   uint32_t quantity;
   uint32_t idx;
};

struct Order {
   uint32_t quantity;
   uint32_t price_idx;
};

using PRICE_MAP = boost::unordered_flat_map<uint32_t, PriceLevel *>;

class OrderBook {
   std::span<Order> orders_;

   std::vector<PriceLevel> bid_levels_{};
   std::vector<PriceLevel> ask_levels_{};
   PRICE_MAP bid_price_map_{};
   PRICE_MAP ask_price_map_{};
   ItchOrderAdd tmp_order_add_{};
   ItchOrderDelete tmp_order_delete_{};

public:
   OrderBook(
         auto orders,
         auto &bid_prices,
         auto &ask_prices) :
         orders_{orders} {
      InitializePrices(bid_levels_, bid_prices, bid_price_map_, 1);
      InitializePrices(ask_levels_, ask_prices, ask_price_map_, 0);
   }

   static void InitializePrices(
         auto &price_levels,
         auto &prices,
         auto &price_map,
         auto bid) {
      price_levels.reserve(prices.size());

      for (uint32_t idx = 0; idx < prices.size(); ++idx) {
         price_levels.push_back(PriceLevel{0, idx | bid << 31});
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
      tmp_order_add_.bid = orig_order->price_idx >> 31;
      tmp_order_add_.quantity = itch_order.quantity;
      tmp_order_add_.price = itch_order.price;
      OrderAddImpl(tmp_order_add_);
   }

   [[nodiscard]] ALWAYS_INLINE
   PriceLevel *PriceLevelFromPrice(const ItchOrderAdd &order) {
      return order.bid ? bid_price_map_[order.price] : ask_price_map_[order.price];
   }

   [[nodiscard]] ALWAYS_INLINE
   PriceLevel *PriceLevelFromIndex(const Order &order) {
      const auto bid = order.price_idx >> 31;
      const auto idx = order.price_idx & 0x7fffffff;
      return bid ? &bid_levels_[idx] : &ask_levels_[idx];
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

      order->quantity = itch_order.quantity;
      order->price_idx = price_level->idx;

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
private:
   const nostromo::Mmap<Order> orders_mmap_;
   const nostromo::Mmap<OrderBook> order_books_mmap_;
   const std::span<Order> orders_;
   const std::span<OrderBook> order_books_;

public:
   explicit OrderBooks(
         const auto max_order_id,
         const auto max_stock_code,
         const auto &stock_prices,
         const size_t page_size = 0u)
         : orders_mmap_{max_order_id + 1u, page_size},
           order_books_mmap_{max_stock_code + 1u, page_size},
           orders_{orders_mmap_.Span()},
           order_books_{order_books_mmap_.Span()} {
      for (auto &[stock_code, pair]: stock_prices) {
         const auto addr = &order_books_[stock_code];
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

} // order_book::itch::v06

#endif //ORDER_BOOK_ITCH_V06_ORDER_BOOK_HPP
