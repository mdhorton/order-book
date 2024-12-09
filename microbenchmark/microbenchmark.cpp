#include <benchmark/benchmark.h>

#include "itch_1/order_book.hpp"

namespace order_book::itch_1 {
static void BM_itch_1_bid_add_delete(benchmark::State &state) {
   const auto base_price = std::rand() % 100'000 + 300'000;
   const auto base_order_id = std::rand() % 1000;
   constexpr auto price_mask = (1 << 17) - 1; // 128k
   constexpr auto order_id_mask = (1 << 10) - 1; // 1k
   uint64_t iters = 0;

   OrderBook book{100, 2500};

   for (auto _: state) {
      ++iters;
      const auto price = static_cast<uint32_t>((iters & price_mask) + base_price);
      const auto order_id = static_cast<uint32_t>((iters & order_id_mask) + base_order_id);
      book.OrderAdd(ItchOrderAdd{1, 1, order_id, 1, 1, price});
      book.OrderDelete(ItchOrderDelete{1, 1, order_id});
      benchmark::ClobberMemory();
   }

   benchmark::DoNotOptimize(book.BestBid());
}

BENCHMARK(BM_itch_1_bid_add_delete);
} // order_book::itch_1

static void BM_div_32(benchmark::State &state) {
   int32_t p1 = std::rand();
   int32_t r1 = 0;

   for (auto _: state) {
      constexpr int32_t div = 100;
      ++p1;
      benchmark::DoNotOptimize(r1 = p1 / div - p1);
      benchmark::ClobberMemory();
   }

   benchmark::DoNotOptimize(r1);
}

BENCHMARK(BM_div_32);
