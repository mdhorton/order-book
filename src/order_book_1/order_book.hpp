#pragma once

#include <cstdint>
#include <stdexcept>

#include "order_book/model.hpp"

namespace nostromo::order_book::order_book_1 {
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

public:
   OrderBook(
      const uint32_t price_divisor,
      const uint32_t price_offset)
      : price_divisor_(price_divisor),
        price_offset_(price_offset) {
   }

   PriceLevel *priceLevel(
      const uint32_t bid,
      const uint32_t price_idx) {
      return bid ? &bid_levels_[price_idx] : &ask_levels_[price_idx];
   }

   [[nodiscard]] uint32_t priceIdx(const uint32_t price) const {
      return price / price_divisor_ - price_offset_;
   }

   void OrderAdd(const ItchOrderAdd &itch_order) {
      const auto price_idx = priceIdx(itch_order.price);

      const auto order = &orders_[itch_order.order_id];
      order->timestamp = itch_order.timestamp;
      order->order_id = itch_order.order_id;
      order->quantity = itch_order.quantity;
      order->price_idx = price_idx;
      order->bid = itch_order.bid;

      const auto price_level = priceLevel(order->bid, price_idx);

      if (price_level->order_count == 0) {
         price_level->head_order_idx = order->order_id;
      }
      else {
         order->prev_idx = price_level->tail_order_idx;
         const auto tail_order = &orders_[price_level->tail_order_idx];
         tail_order->next_idx = order->order_id;
      }

      price_level->price = itch_order.price;
      price_level->tail_order_idx = order->order_id;
      price_level->quantity += order->quantity;
      ++price_level->order_count;
      price_level->bid += order->bid;

      if (order->bid) {
         if (itch_order.price > best_bid_) best_bid_ = itch_order.price;
         ++bid_count_;
      }
      else {
         if (itch_order.price < best_ask_) best_ask_ = itch_order.price;
         ++ask_count_;
      }
   }

   // void OrderExecuted(const ItchOrderExecuted &order) {
   // }

   // void OrderExecutedPrice(const ItchOrderExecutedPrice &order) {
   // }

   // void OrderCancel(const ItchOrderCancel &order) {
   // }

   void OrderDelete(const ItchOrderDelete &itch_order) {
      const auto order = &orders_[itch_order.order_id];
      const auto price_level = priceLevel(order->bid, order->price_idx);

      --price_level->order_count;
      price_level->quantity -= order->quantity;

      if (order->bid) --bid_count_;
      else --ask_count_;

      if (price_level->order_count == 0) {
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

            if (order->price_idx == 0) {
               throw std::runtime_error("bad bid price index");
            }

            auto price_idx = order->price_idx - 1;
            while (true) {
               if (const auto pl = priceLevel(order->bid, price_idx);
                  pl->order_count > 0) {
                  best_bid_ = pl->price;
                  return;
               }
               if (price_idx == 0) {
                  throw std::runtime_error("best bid not found");
               }
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

         if (order->price_idx == MAX_PRICE_IDX) {
            throw std::runtime_error("bad ask price index");
         }

         auto price_idx = order->price_idx + 1;
         while (true) {
            if (const auto pl = priceLevel(order->bid, price_idx);
               pl->order_count > 0) {
               best_ask_ = pl->price;
               return;
            }
            if (price_idx == MAX_PRICE_IDX) {
               throw std::runtime_error("best ask not found");
            }
            ++price_idx;
         }
      }

      // else price_level->order_count > 0

      // was it the head order?
      if (price_level->head_order_idx == order->order_id) {
         // make the next order the new head order.
         const auto next_order = &orders_[order->next_idx];
         price_level->head_order_idx = next_order->order_id;
      }
      // else was it the tail order?
      else if (price_level->tail_order_idx == order->order_id) {
         // make the prev order the new tail order.
         const auto prev_order = &orders_[order->prev_idx];
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

   // void OrderReplace(const ItchOrderReplace &order) {
   // }

   [[nodiscard]] uint32_t BestBid() const {
      return best_bid_;
   }

   [[nodiscard]] uint32_t BestAsk() const {
      return best_ask_;
   }

   // void HandleBidTrade(const NewOrder &new_order, const int32_t price_idx) {
   //    auto quantity = new_order.quantity;
   //
   //    for (auto price = best_ask_; price <= price_idx; ++price) {
   //       const auto price_level = &price_levels_[price];
   //       if (price_level->order_count == 0) {
   //          continue;
   //       }
   //
   //       best_bid_ = price;
   //       // we need this here so that we can find the best price in case the
   //       // previous iteration swept the price level and quantity == 0.
   //       if (quantity == 0) {
   //          return;
   //       }
   //
   //       while (quantity > 0) {
   //          const auto order = &orders_[price_level->head_order_idx];
   //
   //          if (order->quantity > quantity) {
   //             order->quantity -= quantity;
   //             price_level->quantity -= quantity;
   //             return;
   //          }
   //
   //          quantity -= order->quantity;
   //          price_level->quantity -= order->quantity;
   //          --price_level->order_count;
   //
   //          if (price_level->order_count > 0) {
   //             price_level->head_order_idx = order->next_idx;
   //          }
   //       }
   //    }
   //
   //    AddNewOrderImpl(new_order, price_idx, quantity);
   // }

   // void HandleAskTrade(const NewOrder &new_order, const int32_t price_idx) {
   //    auto quantity = new_order.quantity;
   //
   //    for (auto price = best_bid_; price >= price_idx; --price) {
   //       const auto price_level = &price_levels_[price];
   //       if (price_level->order_count == 0) {
   //          continue;
   //       }
   //
   //       best_bid_ = price;
   //       // we need this here so that we can find the best price in case the
   //       // previous iteration swept the price level and quantity == 0.
   //       if (quantity == 0) {
   //          return;
   //       }
   //
   //       while (quantity > 0) {
   //          const auto order = &orders_[price_level->head_order_idx];
   //
   //          if (order->quantity > quantity) {
   //             order->quantity -= quantity;
   //             price_level->quantity -= quantity;
   //             return;
   //          }
   //
   //          quantity -= order->quantity;
   //          price_level->quantity -= order->quantity;
   //          --price_level->order_count;
   //
   //          if (price_level->order_count > 0) {
   //             price_level->head_order_idx = order->next_idx;
   //          }
   //       }
   //    }
   //
   //    AddNewOrderImpl(new_order, price_idx, quantity);
   // }
};
} // nostromo::order_book::order_book_1
