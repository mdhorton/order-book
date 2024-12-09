#pragma once

#include <cstdint>
#include <cassert>

#include "order_book/model.hpp"

namespace order_book::itch_1 {
class OrderBook {
   const uint32_t price_divisor_;
   const uint32_t price_offset_;

   Order orders_[MAX_ORDERS]{};
   PriceLevel bid_levels_[MAX_PRICE_LEVELS]{};
   PriceLevel ask_levels_[MAX_PRICE_LEVELS]{};

   uint32_t best_bid_{0};
   uint32_t best_ask_{MAX_PRICE};

   uint32_t bid_count_{0};
   uint32_t ask_count_{0};

   ItchOrderAdd tmp_order_add_{};
   ItchOrderDelete tmp_order_delete_{};

public:
   OrderBook(
      const uint32_t price_divisor,
      const uint32_t price_offset)
      : price_divisor_(price_divisor),
        price_offset_(price_offset) {
   }

   void OrderAdd(const ItchOrderAdd &itch_order) {
      const auto price_idx = GetPriceIdx(itch_order.price);
      const auto order = GetOrder(itch_order.order_id);
      const auto price_level = GetPriceLevel(itch_order.bid, price_idx);

      order->timestamp = itch_order.timestamp;
      order->order_id = itch_order.order_id;
      order->quantity = itch_order.quantity;
      order->price_idx = price_idx;
      order->bid = itch_order.bid;

      if (price_level->order_count == 0) {
         price_level->head_order_idx = itch_order.order_id;
      }
      else {
         order->prev_idx = price_level->tail_order_idx;
         const auto tail_order = GetOrder(price_level->tail_order_idx);
         tail_order->next_idx = itch_order.order_id;
      }

      price_level->price = itch_order.price;
      price_level->tail_order_idx = itch_order.order_id;
      price_level->quantity += itch_order.quantity;
      ++price_level->order_count;
      price_level->bid += itch_order.bid;

      if (itch_order.bid) {
         if (itch_order.price > best_bid_) best_bid_ = itch_order.price;
         ++bid_count_;
      }
      else {
         if (itch_order.price < best_ask_) best_ask_ = itch_order.price;
         ++ask_count_;
      }
   }

   void OrderExecuted(const ItchOrderExecuted &itch_order) {
      const auto order = GetOrder(itch_order.order_id);
      const auto price_level = GetPriceLevel(order->bid, order->price_idx);

      assert(price_level->quantity >= itch_order.quantity);
      price_level->quantity -= itch_order.quantity;
      if (price_level->quantity == 0) {
         decrementBidAskCount(order->bid);
         findBestBidAsk(order, price_level);
         return;
      }

      assert(order->quantity >= itch_order.quantity);
      order->quantity -= itch_order.quantity;
      if (order->quantity == 0) {
         decrementBidAskCount(order->bid);
         removeOrder(order, price_level);
      }
   }

   void OrderExecutedPrice(const ItchOrderExecutedPrice &itch_order) {
      const auto order = GetOrder(itch_order.order_id);
      const auto price_level = GetPriceLevel(order->bid, order->price_idx);

      assert(price_level->quantity >= itch_order.quantity);
      price_level->quantity -= itch_order.quantity;
      if (price_level->quantity == 0) {
         decrementBidAskCount(order->bid);
         findBestBidAsk(order, price_level);
         return;
      }

      assert(order->quantity >= itch_order.quantity);
      order->quantity -= itch_order.quantity;
      if (order->quantity == 0) {
         decrementBidAskCount(order->bid);
         removeOrder(order, price_level);
      }
   }

   void OrderCancel(const ItchOrderCancel &itch_order) {
      const auto order = GetOrder(itch_order.order_id);
      const auto price_level = GetPriceLevel(order->bid, order->price_idx);

      assert(order->quantity > itch_order.quantity);
      assert(price_level->quantity > itch_order.quantity);

      order->quantity -= itch_order.quantity;
      price_level->quantity -= itch_order.quantity;
   }

   void OrderDelete(const ItchOrderDelete &itch_order) {
      const auto order = GetOrder(itch_order.order_id);
      const auto price_level = GetPriceLevel(order->bid, order->price_idx);

      decrementBidAskCount(order->bid);

      assert(price_level->order_count > 0);
      assert(price_level->quantity >= order->quantity);

      --price_level->order_count;
      price_level->quantity -= order->quantity;

      if (price_level->order_count == 0) {
         findBestBidAsk(order, price_level);
         return;
      }

      removeOrder(order, price_level);
   }

   void OrderReplace(const ItchOrderReplace &itch_order) {
      tmp_order_delete_.stock_code = itch_order.stock_code;
      tmp_order_delete_.timestamp = itch_order.timestamp;
      tmp_order_delete_.order_id = itch_order.orig_order_id;
      OrderDelete(tmp_order_delete_);
      const auto orig_order = GetOrder(itch_order.orig_order_id);
      tmp_order_add_.stock_code = itch_order.stock_code;
      tmp_order_add_.timestamp = itch_order.timestamp;
      tmp_order_add_.order_id = itch_order.new_order_id;
      tmp_order_add_.bid = orig_order->bid;
      tmp_order_add_.quantity = itch_order.quantity;
      tmp_order_add_.price = itch_order.price;
      OrderAdd(tmp_order_add_);
   }

   [[nodiscard]] uint32_t BestBid() const {
      return best_bid_;
   }

   [[nodiscard]] uint32_t BestAsk() const {
      return best_ask_;
   }

   [[nodiscard]] uint32_t GetBidCount() const {
      return bid_count_;
   }

   [[nodiscard]] uint32_t GetAskCount() const {
      return ask_count_;
   }

   [[nodiscard]] PriceLevel *GetPriceLevel(
      const uint32_t bid,
      const uint32_t price_idx) {
      assert(price_idx <= MAX_PRICE_IDX);
      return bid ? &bid_levels_[price_idx] : &ask_levels_[price_idx];
   }

   [[nodiscard]] Order *GetOrder(const uint32_t order_id) {
      assert(order_id <= MAX_ORDER_IDX);
      return &orders_[order_id];
   }

   [[nodiscard]] uint32_t GetPriceIdx(const uint32_t price) const {
      assert(price <= MAX_PRICE);
      return price / price_divisor_ - price_offset_;
   }

private:
   void findBestBidAsk(const Order *order, const PriceLevel *price_level) {
      // this price level is now empty.
      // do we need a new best bid?
      if (order->bid) {
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
            if (const auto pl = GetPriceLevel(order->bid, price_idx);
               pl->order_count > 0) {
               best_bid_ = pl->price;
               return;
            }
            assert(order->price_idx > 0);
            --price_idx;
         }
      }

      // do we need a new best ask?
      if (price_level->price != best_ask_) {
         return;
      }

      if (ask_count_ == 0) {
         best_ask_ = MAX_PRICE;
         return;
      }

      assert(order->price_idx < MAX_ORDER_IDX);

      auto price_idx = order->price_idx + 1;
      while (true) {
         if (const auto pl = GetPriceLevel(order->bid, price_idx);
            pl->order_count > 0) {
            best_ask_ = pl->price;
            return;
         }
         assert(order->price_idx < MAX_ORDER_IDX);
         ++price_idx;
      }
   }

   void removeOrder(const Order *order, PriceLevel *price_level) {
      // is it the head order?
      if (price_level->head_order_idx == order->order_id) {
         // make the next order the new head order.
         const auto next_order = GetOrder(order->next_idx);
         price_level->head_order_idx = next_order->order_id;
      }
      // else is it the tail order?
      else if (price_level->tail_order_idx == order->order_id) {
         // make the prev order the new tail order.
         const auto prev_order = GetOrder(order->prev_idx);
         price_level->tail_order_idx = prev_order->order_id;
      }
      // otherwise it's somewhere in the middle.
      else {
         const auto prev_order = GetOrder(order->prev_idx);
         const auto next_order = GetOrder(order->next_idx);
         prev_order->next_idx = next_order->order_id;
         next_order->prev_idx = prev_order->order_id;
      }
   }

   void decrementBidAskCount(const uint32_t bid) {
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
} // order_book::itch_1
