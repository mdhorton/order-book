#include <benchmark/benchmark.h>

#include "order_book_1/order_book.hpp"

namespace nostromo::order_book::order_book_1 {
static void BM_order_book_1_bid_cancel(benchmark::State &state) {
   printf("%lu\n", sizeof(NewOrder));
   const auto base_price = std::rand() % 100000 + 300000;
   const auto base_order_id = std::rand() % 1000;
   OrderBook book{100, 2500};

   for (auto _: state) {
      const auto price = static_cast<int32_t>(state.iterations() % 100000 + base_price);
      const auto order_id = static_cast<uint32_t>(state.iterations() % 10000 + base_order_id);
      book.HandleNewOrder(NewOrder{price, order_id, 1, 1, true});
      book.HandleCancelOrder(CancelOrder{order_id});
   }

   benchmark::DoNotOptimize(book.BestBid());
}

static void BM_div_32(benchmark::State &state) {
   int32_t p1 = std::rand();
   int32_t r1 = 0;

   for (auto _: state) {
      constexpr int32_t div = 100;
      ++p1;
      benchmark::DoNotOptimize(r1 = p1 / div - p1);
   }

   benchmark::DoNotOptimize(r1);
}

BENCHMARK(BM_order_book_1_bid_cancel)->MinTime(1);
BENCHMARK(BM_div_32);
} // nostromo::order_book::order_book_1
