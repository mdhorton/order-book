#ifndef ORDER_BOOK_ITCH_v02_ORDER_BOOK_HPP
#define ORDER_BOOK_ITCH_v02_ORDER_BOOK_HPP

#include <cstdint>
#include <cassert>
#include <span>
#include <map>
#include <set>

#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>

#include "common.hpp"
#include "itch/itch.hpp"
#include "itch/v02/model.hpp"

#define MAP boost::unordered_flat_map
#define SET boost::unordered_flat_set

namespace order_book::itch::v02 {

class OrderBook {
   std::span<Order> orders_;
   const size_t bid_price_cnt;
   const size_t ask_price_cnt;

   std::vector<PriceLevel> bid_levels_{};
   std::vector<PriceLevel> ask_levels_{};
   MAP<uint32_t, PriceLevel *> bid_price_map_{};
   MAP<uint32_t, PriceLevel *> ask_price_map_{};
   uint32_t bid_count_{0};
   uint32_t ask_count_{0};
   uint32_t best_bid_{0};
   uint32_t best_ask_{MAX_PRICE + 1};
   ItchOrderAdd tmp_order_add_{};
   ItchOrderDelete tmp_order_delete_{};

public:
   OrderBook(
         std::span<Order> orders,
         const std::set<uint32_t> &bid_prices,
         const std::set<uint32_t> &ask_prices) :
         orders_{orders},
         bid_price_cnt{bid_prices.size()},
         ask_price_cnt{ask_prices.size()} {
      {
         bid_levels_.reserve(bid_price_cnt);
         uint16_t idx = 0;

         for (auto price: bid_prices) {
            bid_levels_.emplace_back();
            auto tmp = &bid_levels_[idx];
            tmp->price = price;
            tmp->idx = idx;
            bid_price_map_[price] = tmp;
            ++idx;
         }
      }

      {
         ask_levels_.reserve(ask_price_cnt);
         uint16_t idx = 0;

         for (auto price: ask_prices) {
            ask_levels_.emplace_back();
            auto tmp = &ask_levels_[idx];
            tmp->price = price;
            tmp->idx = idx;
            ask_price_map_[price] = tmp;
            ++idx;
         }
      }
   }

   void OrderAdd(const ItchOrderAdd &itch_order) {
      OrderAddImpl(itch_order);
   }

   void OrderExecuted(const ItchOrderExecuted &itch_order) {
      const auto order = OrderFromId(itch_order.order_id);
      const auto price_level = PriceLevelFromIndex(order->bid, order->price_idx);

      assert(price_level->quantity >= itch_order.quantity);
      price_level->quantity -= itch_order.quantity;
      if (price_level->quantity == 0) {
         DecrementBidAskCount(order->bid);
         CheckBestBidAsk(order, price_level);
         return;
      }

      assert(order->quantity >= itch_order.quantity);
      order->quantity -= itch_order.quantity;
      if (order->quantity == 0) {
         DecrementBidAskCount(order->bid);
         RemoveOrder(order, price_level);
      }
   }

   void OrderCancel(const ItchOrderCancel &itch_order) {
      const auto order = OrderFromId(itch_order.order_id);
      const auto price_level = PriceLevelFromIndex(order->bid, order->price_idx);

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
      tmp_order_delete_.timestamp = itch_order.timestamp;
      tmp_order_delete_.order_id = itch_order.order_id;
      OrderDeleteImpl(tmp_order_delete_);
      const auto orig_order = OrderFromId(itch_order.order_id);
      tmp_order_add_.stock_code = itch_order.stock_code;
      tmp_order_add_.timestamp = itch_order.timestamp;
      tmp_order_add_.order_id = itch_order.new_order_id;
      tmp_order_add_.bid = orig_order->bid;
      tmp_order_add_.quantity = itch_order.quantity;
      tmp_order_add_.price = itch_order.price;
      OrderAddImpl(tmp_order_add_);
   }

   [[nodiscard]] ALWAYS_INLINE
   uint32_t BestBid() const {
      return best_bid_;
   }

   [[nodiscard]] ALWAYS_INLINE
   uint32_t BestAsk() const {
      return best_ask_;
   }

   [[nodiscard]] ALWAYS_INLINE
   uint32_t BidCount() const {
      return bid_count_;
   }

   [[nodiscard]] ALWAYS_INLINE
   uint32_t AskCount() const {
      return ask_count_;
   }

   [[nodiscard]] ALWAYS_INLINE
   PriceLevel *PriceLevelFromPrice(
         const uint8_t bid,
         const uint32_t price) {
      return bid ? bid_price_map_[price] : ask_price_map_[price];
   }

   [[nodiscard]] ALWAYS_INLINE
   PriceLevel *PriceLevelFromIndex(
         const uint8_t bid,
         const uint32_t idx) {
      return bid ? &bid_levels_[idx] : &ask_levels_[idx];
   }

   [[nodiscard]] ALWAYS_INLINE
   Order *OrderFromId(const uint32_t order_id) {
      return &orders_[order_id];
   }

private:
   ALWAYS_INLINE
   void OrderAddImpl(const ItchOrderAdd &itch_order) {
      auto price_level = PriceLevelFromPrice(itch_order.bid, itch_order.price);
      auto order = OrderFromId(itch_order.order_id);

      order->timestamp = itch_order.timestamp;
      order->order_id = itch_order.order_id;
      order->quantity = itch_order.quantity;
      order->price_idx = price_level->idx;
      order->bid = itch_order.bid;

      if (price_level->order_count == 0) {
         price_level->head_order_idx = itch_order.order_id;
      }
      else {
         order->prev_idx = price_level->tail_order_idx;
         const auto tail_order = OrderFromId(price_level->tail_order_idx);
         tail_order->next_idx = itch_order.order_id;
      }

      price_level->tail_order_idx = itch_order.order_id;
      price_level->quantity += itch_order.quantity;
      ++price_level->order_count;

      if (itch_order.bid) {
         if (itch_order.price > best_bid_) best_bid_ = itch_order.price;
         ++bid_count_;
      }
      else {
         if (itch_order.price < best_ask_) best_ask_ = itch_order.price;
         ++ask_count_;
      }
   }

   ALWAYS_INLINE
   void OrderDeleteImpl(const ItchOrderDelete &itch_order) {
      const auto order = OrderFromId(itch_order.order_id);
      const auto price_level = PriceLevelFromIndex(order->bid, order->price_idx);

      DecrementBidAskCount(order->bid);

      assert(price_level->order_count > 0);
      assert(price_level->quantity >= order->quantity);

      --price_level->order_count;
      price_level->quantity -= order->quantity;

      if (price_level->order_count == 0) {
         CheckBestBidAsk(order, price_level);
         return;
      }

      RemoveOrder(order, price_level);
   }

   ALWAYS_INLINE
   void CheckBestBidAsk(const Order *order, const PriceLevel *price_level) {
      // this price level is now empty.
      // do we need a new best bid?
      if (order->bid) {
         assert(bid_price_cnt > 0);
         if (price_level->price != best_bid_) {
            return;
         }

         if (bid_count_ == 0) {
            best_bid_ = 0;
            return;
         }

         assert(order->price_idx > 0);

         auto price_idx = order->price_idx - 1;
         while (true) {
            if (const auto pl = PriceLevelFromIndex(order->bid, price_idx);
                  pl->order_count > 0) {
               best_bid_ = pl->price;
               return;
            }
            assert(order->price_idx > 0);
            --price_idx;
         }
      }

      // do we need a new best ask?
      assert(ask_price_cnt > 0);
      if (price_level->price != best_ask_) {
         return;
      }

      if (ask_count_ == 0) {
         best_ask_ = MAX_PRICE + 1;
         return;
      }

      assert(order->price_idx < ask_price_cnt - 1);

      auto price_idx = order->price_idx + 1;
      while (true) {
         if (const auto pl = PriceLevelFromIndex(order->bid, price_idx);
               pl->order_count > 0) {
            best_ask_ = pl->price;
            return;
         }
         assert(order->price_idx < ask_price_cnt - 1);
         ++price_idx;
      }
   }

   ALWAYS_INLINE
   void RemoveOrder(const Order *order, PriceLevel *price_level) {
      // is it the head order?
      if (price_level->head_order_idx == order->order_id) {
         // make the next order the new head order.
         const auto next_order = OrderFromId(order->next_idx);
         price_level->head_order_idx = next_order->order_id;
      }
         // else is it the tail order?
      else if (price_level->tail_order_idx == order->order_id) {
         // make the prev order the new tail order.
         const auto prev_order = OrderFromId(order->prev_idx);
         price_level->tail_order_idx = prev_order->order_id;
      }
         // otherwise it's somewhere in the middle.
      else {
         const auto prev_order = OrderFromId(order->prev_idx);
         const auto next_order = OrderFromId(order->next_idx);
         prev_order->next_idx = next_order->order_id;
         next_order->prev_idx = prev_order->order_id;
      }
   }

   ALWAYS_INLINE
   void DecrementBidAskCount(const uint32_t bid) {
      if (bid) {
         assert(bid_count_ > 0);
         --bid_count_;
      }
      else {
         assert(ask_count_ > 0);
         --ask_count_;
      }
   }
};

class OrderBooks {
   std::vector<Order> orders_{};
   std::vector<OrderBook> order_books_{};

public:
   OrderBooks(
         const uint32_t max_order_id,
         const std::map<uint16_t, std::pair<std::set<uint32_t>, std::set<uint32_t>>> &stock_prices) {
      orders_.reserve(max_order_id + 1);

      for (uint32_t x = 0; x <= max_order_id; ++x) {
         orders_.emplace_back();
      }

      const uint16_t max_stock_id = stock_prices.rbegin()->first;
      order_books_.reserve(max_stock_id + 1);

      for (uint16_t idx = 0; idx <= max_stock_id; ++idx) {
         auto res = stock_prices.find(idx);
         if (res == stock_prices.end()) {
            order_books_.emplace_back(orders_, std::set<uint32_t>{}, std::set<uint32_t>{});
            continue;
         }
         auto pair = res->second;
         order_books_.emplace_back(orders_, pair.first, pair.second);
      }
   }

   void OrderAdd(const ItchOrderAdd &order) {
      order_books_[order.stock_code].OrderAdd(order);
   }

   void OrderExecuted(const ItchOrderExecuted &order) {
      order_books_[order.stock_code].OrderExecuted(order);
   }

   void OrderCancel(const ItchOrderCancel &order) {
      order_books_[order.stock_code].OrderCancel(order);
   }

   void OrderDelete(const ItchOrderDelete &order) {
      order_books_[order.stock_code].OrderDelete(order);
   }

   void OrderReplace(const ItchOrderReplace &order) {
      order_books_[order.stock_code].OrderReplace(order);
   }
};

} // order_book::itch::v02

#endif //ORDER_BOOK_ITCH_v02_ORDER_BOOK_HPP
