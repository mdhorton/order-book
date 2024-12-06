#pragma once

#include <cstdint>
#include <array>

#include "order_book/model.hpp"

namespace nostromo::order_book::order_book_1 {
class OrderBook {
   const int32_t price_divisor_;
   const int32_t price_offset_;

   std::array<PriceLevel, MAX_PRICE_LEVELS> price_levels_{};
   std::array<Order, MAX_ORDERS> orders_{};
   int32_t best_bid_{0};
   int32_t best_ask_{MAX_PRICE_LEVELS - 1};
   uint32_t bid_count_{0};
   uint32_t ask_count_{0};

public:
   OrderBook(
      const int32_t price_divisor,
      const int32_t price_offset)
      : price_divisor_(price_divisor),
        price_offset_(price_offset) {
   }

   void HandleNewOrder(const NewOrder &new_order) {
      const auto price_idx = new_order.price / price_divisor_ - price_offset_;

      if (new_order.bid) {
         if (price_idx < best_ask_) {
            AddNewOrderImpl(new_order, price_idx, new_order.quantity);
         }
         else {
            HandleBidTrade(new_order, price_idx);
         }
      }
      else {
         if (price_idx > best_bid_) {
            AddNewOrderImpl(new_order, price_idx, new_order.quantity);
         }
         else {
            HandleAskTrade(new_order, price_idx);
         }
      }
   }

   void HandleCancelOrder(const CancelOrder &cancel_order) {
      const auto order = &orders_[cancel_order.order_id];
      const auto price_level = &price_levels_[order->price_idx];

      --price_level->order_count;
      price_level->quantity -= order->quantity;

      if (order->price_idx >= best_ask_) {
         --ask_count_;
      }
      else {
         --bid_count_;
      }

      if (price_level->order_count == 0) {
         // this price level is now empty. was it the best price?
         // if so, then we need a new best price.
         if (order->price_idx == best_bid_) {
            if (bid_count_ > 0) {
               for (auto price = order->price_idx - 1; price >= 0; --price) {
                  if (const auto pl = &price_levels_[price]; pl->order_count > 0) {
                     best_bid_ = price;
                     return;
                  }
               }
            }
            best_bid_ = 0;
         }
         else if (order->price_idx == best_ask_) {
            if (ask_count_ > 0) {
               for (auto price = order->price_idx + 1; price < best_ask_; ++price) {
                  if (const auto pl = &price_levels_[price]; pl->order_count > 0) {
                     best_ask_ = price;
                     return;
                  }
               }
            }
            best_ask_ = MAX_PRICE_LEVELS - 1;
         }
      }
      else {
         if (price_level->order_count > 0) {
            // was it the head order?
            if (price_level->head_order_idx == order->order_id) {
               // make the next order the new head order.
               const auto next_order = &orders_[order->next_idx];
               next_order->prev_idx = next_order->order_id;
               price_level->head_order_idx = next_order->order_id;
            }
            // else was it the tail order?
            else if (price_level->tail_order_idx == order->order_id) {
               // make the prev order the new tail order.
               const auto prev_order = &orders_[order->prev_idx];
               prev_order->next_idx = prev_order->order_id;
               price_level->tail_order_idx = prev_order->order_id;
            }
            // otherwise it was somewhere in the middle.
            else {
               const auto prev_order = &orders_[order->prev_idx];
               const auto next_order = &orders_[order->next_idx];
               prev_order->next_idx = next_order->order_id;
               next_order->prev_idx = prev_order->order_id;
            }
         }
      }
   }

   [[nodiscard]] int32_t BestBid() const {
      return (best_bid_ + price_offset_) * price_divisor_;
   }

   [[nodiscard]] int32_t BestAsk() const {
      return (best_ask_ + price_offset_) * price_divisor_;
   }

private:
   void AddNewOrderImpl(
      const NewOrder &new_order,
      const int32_t price_idx,
      const uint32_t quantity) {
      const auto price_level = &price_levels_[price_idx];
      const auto order = &orders_[new_order.order_id];
      order->order_id = new_order.order_id;
      order->timestamp = new_order.timestamp;
      order->quantity = quantity;
      order->next_idx = new_order.order_id;
      order->price_idx = price_idx;

      if (new_order.bid) {
         ++bid_count_;
      }
      else {
         ++ask_count_;
      }

      // will this be the only order at this price level?
      if (price_level->order_count == 0) {
         order->prev_idx = order->order_id;
         price_level->head_order_idx = order->order_id;

         // do we have a new best price?
         if (new_order.bid && price_idx > best_bid_) {
            best_bid_ = price_idx;
         }
         else if (!new_order.bid && price_idx < best_ask_) {
            best_ask_ = price_idx;
         }
      }
      // else there are other orders at this price level.
      else {
         order->prev_idx = price_level->tail_order_idx;
         const auto tail_order = &orders_[price_level->tail_order_idx];
         tail_order->next_idx = order->order_id;
      }

      price_level->tail_order_idx = order->order_id;
      ++price_level->order_count;
      price_level->quantity += quantity;
   }

   void HandleBidTrade(const NewOrder &new_order, const int32_t price_idx) {
      auto quantity = new_order.quantity;

      for (auto price = best_ask_; price <= price_idx; ++price) {
         const auto price_level = &price_levels_[price];
         if (price_level->order_count == 0) {
            continue;
         }

         best_bid_ = price;
         // we need this here so that we can find the best price in case the
         // previous iteration swept the price level and quantity == 0.
         if (quantity == 0) {
            return;
         }

         while (quantity > 0) {
            const auto order = &orders_[price_level->head_order_idx];

            if (order->quantity > quantity) {
               order->quantity -= quantity;
               price_level->quantity -= quantity;
               return;
            }

            quantity -= order->quantity;
            price_level->quantity -= order->quantity;
            --price_level->order_count;

            if (price_level->order_count > 0) {
               price_level->head_order_idx = order->next_idx;
            }
         }
      }

      AddNewOrderImpl(new_order, price_idx, quantity);
   }

   void HandleAskTrade(const NewOrder &new_order, const int32_t price_idx) {
      auto quantity = new_order.quantity;

      for (auto price = best_bid_; price >= price_idx; --price) {
         const auto price_level = &price_levels_[price];
         if (price_level->order_count == 0) {
            continue;
         }

         best_bid_ = price;
         // we need this here so that we can find the best price in case the
         // previous iteration swept the price level and quantity == 0.
         if (quantity == 0) {
            return;
         }

         while (quantity > 0) {
            const auto order = &orders_[price_level->head_order_idx];

            if (order->quantity > quantity) {
               order->quantity -= quantity;
               price_level->quantity -= quantity;
               return;
            }

            quantity -= order->quantity;
            price_level->quantity -= order->quantity;
            --price_level->order_count;

            if (price_level->order_count > 0) {
               price_level->head_order_idx = order->next_idx;
            }
         }
      }

      AddNewOrderImpl(new_order, price_idx, quantity);
   }
};
} // nostromo::order_book::order_book_1
