#ifndef ORDER_BOOK_ITCH_V26_ORDER_BOOK_HPP
#define ORDER_BOOK_ITCH_V26_ORDER_BOOK_HPP

#include "itch/defs.hpp"
#include "itch/common.hpp"
#include "itch/usings.hpp"
#include "itch/itch.hpp"

#include "nostromo/mmap.hpp"

#include <cstdint>
#include <cassert>
#include <vector>
#include <span>

// copied from v18.
// removed Order->level and added PriceLevel->idx & Order->price_idx.
namespace order_book::itch::v26 {

struct PriceLevel {
   uint32_t quantity;
   uint32_t order_count;
   uint32_t price;
   uint32_t idx;
};

struct Order {
   uint32_t quantity;
   uint32_t price_idx;
   uint8_t bid;
} PACKED;

struct SideData {
   const PriceLevel *last_level;
   PriceLevel *best_level;
};

class OrderBook {
   const std::span<Order> orders_;
   const std::span<PriceLevel> levels_[2];
   SideData side_data_[2];

   ItchOrderAddIdx tmp_order_add_{};
   ItchOrderDelete tmp_order_delete_{};

public:
   OrderBook(
         const std::span<Order> orders,
         const std::span<PriceLevel> ask_levels,
         const std::span<PriceLevel> bid_levels,
         const std::vector<uint32_t> &asks,
         const std::vector<uint32_t> &bids)
         : orders_{orders},
           levels_{ask_levels, bid_levels},
           side_data_{
                 {asks.empty() ? nullptr : &ask_levels.back(), nullptr},
                 {bids.empty() ? nullptr : &bid_levels.back(), nullptr}} {
      InitializePrices(asks, levels_[0]);
      InitializePrices(bids, levels_[1]);
   }

   static void InitializePrices(
         const auto &prices,
         auto &levels) {
      for (auto idx = 0u; idx < prices.size(); ++idx) {
         auto &level = levels[idx];
         level.price = prices[idx];
         level.idx = idx;
      }
   }

   ALWAYS_INLINE
   void OrderAdd(const ItchOrderAddIdx &itch_order) {
      OrderAddImpl(itch_order);
   }

   ALWAYS_INLINE
   void OrderExecuted(const ItchOrderExecuted &itch_order) {
      const auto order = OrderFromId(itch_order.order_id);
      const auto level = PriceLevelFromIndex(*order);
      assert(order->quantity >= itch_order.quantity);
      assert(level->quantity >= itch_order.quantity);
      assert(level->order_count > 0u);
      order->quantity -= itch_order.quantity;
      level->quantity -= itch_order.quantity;
      CheckBestPriceLevel(level, order->bid);
   }

   ALWAYS_INLINE
   void OrderCancel(const ItchOrderCancel &itch_order) {
      const auto order = OrderFromId(itch_order.order_id);
      const auto level = PriceLevelFromIndex(*order);
      assert(order->quantity > itch_order.quantity);
      assert(level->quantity > itch_order.quantity);
      assert(level->order_count > 0u);
      order->quantity -= itch_order.quantity;
      level->quantity -= itch_order.quantity;
   }

   ALWAYS_INLINE
   void OrderDelete(const ItchOrderDelete &itch_order) {
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
      return &levels_[order.bid][order.price_idx];
   }

   [[nodiscard]] ALWAYS_INLINE
   PriceLevel *PriceLevelFromIndex(const Order &order) {
      return &levels_[order.bid][order.price_idx];
   }

   [[nodiscard]] ALWAYS_INLINE
   Order *OrderFromId(const uint32_t order_id) const {
      return &orders_[order_id];
   }

private:
   ALWAYS_INLINE
   void OrderAddImpl(const ItchOrderAddIdx &itch_order) {
      const auto order = OrderFromId(itch_order.order_id);
      const auto level = PriceLevelFromPrice(itch_order);

      order->quantity = itch_order.quantity;
      order->price_idx = level->idx;
      order->bid = itch_order.bid;

      // is this level the new best level?
      if (level->quantity == 0u) {
         const auto side_data = &side_data_[order->bid];
         if (side_data->best_level == nullptr ||
             (order->bid == 0u && itch_order.price < side_data->best_level->price) ||
             (order->bid != 0u && itch_order.price > side_data->best_level->price)) {
            side_data->best_level = level;
         }
      }

      level->quantity += order->quantity;
      ++level->order_count;
   }

   ALWAYS_INLINE
   void OrderDeleteImpl(const ItchOrderDelete &itch_order) {
      const auto order = OrderFromId(itch_order.order_id);
      const auto level = PriceLevelFromIndex(*order);
      assert(level->quantity >= order->quantity);
      assert(level->order_count > 0u);
      order->quantity = 0u;
      level->quantity -= order->quantity;
      --level->order_count;
      CheckBestPriceLevel(level, order->bid);
   }

   ALWAYS_INLINE
   void CheckBestPriceLevel(PriceLevel *level, const uint8_t bid) {
      const auto side_data = &side_data_[bid];

      // is this level empty and is it the best price level?
      if (level->quantity == 0u && level == side_data->best_level) {
         for (auto pl = level; pl != side_data->last_level; ++pl) {
            if (pl->quantity != 0u) {
               side_data->best_level = pl;
               return;
            }
         }

         // all price levels are empty for this side.
         side_data->best_level = nullptr;
      }
   }
};

class OrderBooks {
   const nos::Mmap<Order> orders_mmap_;
   const nos::Mmap<OrderBook> order_books_mmap_;
   const nos::Mmap<PriceLevel> levels_mmap_;
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
         const auto ask_levels = levels_.subspan(offset, asks.size());
         offset += asks.size();
         const auto bid_levels = levels_.subspan(offset, bids.size());
         offset += bids.size();
         const auto addr = &order_books_[stock_code];
         new(addr) OrderBook{orders_, ask_levels, bid_levels, asks, bids};
      }
   }

   INLINE
   void OrderAdd(const ItchOrderAddIdx &order) const {
      auto &order_book = order_books_[order.stock_code];
      order_book.OrderAdd(order);
   }

   INLINE
   void OrderExecuted(const ItchOrderExecuted &order) const {
      auto &order_book = order_books_[order.stock_code];
      order_book.OrderExecuted(order);
   }

   INLINE
   void OrderCancel(const ItchOrderCancel &order) const {
      auto &order_book = order_books_[order.stock_code];
      order_book.OrderCancel(order);
   }

   INLINE
   void OrderDelete(const ItchOrderDelete &order) const {
      auto &order_book = order_books_[order.stock_code];
      order_book.OrderDelete(order);
   }

   INLINE
   void OrderReplace(const ItchOrderReplaceIdx &order) const {
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

} // order_book::itch::v26

#endif //ORDER_BOOK_ITCH_V26_ORDER_BOOK_HPP
