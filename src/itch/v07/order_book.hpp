#ifndef ORDER_BOOK_ITCH_V07_ORDER_BOOK_HPP
#define ORDER_BOOK_ITCH_V07_ORDER_BOOK_HPP

#include <cstdint>
#include <cassert>
#include <span>

#include <boost/unordered/unordered_flat_map.hpp>

#include "nostromo/mmap.hpp"

#include "itch/itch.hpp"

namespace order_book::itch::v07 {

template<typename K, typename V>
using MAP = boost::unordered_flat_map<K, V>;

struct alignas(4) PriceLevel {
   uint32_t quantity;
};

struct alignas (16) Order {
   PriceLevel* level;
   uint32_t quantity;
   uint8_t bid;
};

class OrderBook {
   std::span<Order> orders_;

   std::vector<PriceLevel> bid_levels_{};
   std::vector<PriceLevel> ask_levels_{};
   MAP<uint32_t, PriceLevel *> bid_price_map_{};
   MAP<uint32_t, PriceLevel *> ask_price_map_{};
   ItchOrderAdd tmp_order_add_{};
   ItchOrderDelete tmp_order_delete_{};

public:
   OrderBook(
         auto orders,
         auto &bid_prices,
         auto &ask_prices) :
         orders_{orders} {
      InitializePrices(bid_levels_, bid_prices, bid_price_map_);
      InitializePrices(ask_levels_, ask_prices, ask_price_map_);
   }

   static void InitializePrices(
         auto &price_levels,
         auto &prices,
         auto &price_map) {
      price_levels.reserve(prices.size());

      for (uint32_t idx = 0; idx < prices.size(); ++idx) {
         price_levels.push_back(PriceLevel{});
         price_map[prices[idx]] = &price_levels.back();
      }
   }

   void OrderAdd(const ItchOrderAdd &itch_order) {
      OrderAddImpl(itch_order);
   }

   void OrderExecuted(const ItchOrderExecuted &itch_order) {
      const auto order = OrderFromId(itch_order.order_id);
      const auto price_level = PriceLevelFromIndex(*order);
      assert(order->quantity >= itch_order.quantity);
      assert(price_level->quantity >= itch_order.quantity);
      order->quantity -= itch_order.quantity;
      price_level->quantity -= itch_order.quantity;
   }

   void OrderCancel(const ItchOrderCancel &itch_order) {
      const auto order = OrderFromId(itch_order.order_id);
      const auto price_level = PriceLevelFromIndex(*order);
      assert(order->quantity > itch_order.quantity);
      assert(price_level->quantity > itch_order.quantity);
      order->quantity -= itch_order.quantity;
      price_level->quantity -= itch_order.quantity;
   }

   void OrderDelete(const ItchOrderDelete &itch_order) {
      OrderDeleteImpl(itch_order);
   }

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

   [[nodiscard]] ALWAYS_INLINE
   PriceLevel *PriceLevelFromIndex(const Order &order) {
      return order.level;
   }

   [[nodiscard]] ALWAYS_INLINE
   Order *OrderFromId(const uint32_t order_id) {
      return &orders_[order_id];
   }

private:
   ALWAYS_INLINE
   void OrderAddImpl(const ItchOrderAdd &itch_order) {
      const auto price_level = PriceLevelFromPrice(itch_order);
      const auto order = OrderFromId(itch_order.order_id);

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
private:
   const nostromo::Mmap<Order> orders_mmap_;
   const nostromo::Mmap<OrderBook> order_books_mmap_;
   const std::span<Order> orders_;
   const std::span<OrderBook> order_books_;

public:
   explicit OrderBooks(
         auto max_order_id,
         auto max_stock_code,
         auto &stock_prices,
         size_t page_size = 0)
         : orders_mmap_{max_order_id + 1u, page_size},
           order_books_mmap_{max_stock_code + 1u, page_size},
           orders_{orders_mmap_.Span()},
           order_books_{order_books_mmap_.Span()} {
      for (auto &[stock_code, pair]: stock_prices) {
         auto addr = &order_books_[stock_code];
         new(addr) OrderBook(orders_, pair.first, pair.second);
      }
   }

   void OrderAdd(const ItchOrderAdd &order) const {
      auto &order_book = order_books_[order.stock_code];
      order_book.OrderAdd(order);
   }

   void OrderExecuted(const ItchOrderExecuted &order) const {
      auto &order_book = order_books_[order.stock_code];
      order_book.OrderExecuted(order);
   }

   void OrderCancel(const ItchOrderCancel &order) const {
      auto &order_book = order_books_[order.stock_code];
      order_book.OrderCancel(order);
   }

   void OrderDelete(const ItchOrderDelete &order) const {
      auto &order_book = order_books_[order.stock_code];
      order_book.OrderDelete(order);
   }

   void OrderReplace(const ItchOrderReplace &order) const {
      auto &order_book = order_books_[order.stock_code];
      order_book.OrderReplace(order);
   }
};

} // order_book::itch::v07

#endif //ORDER_BOOK_ITCH_V07_ORDER_BOOK_HPP
