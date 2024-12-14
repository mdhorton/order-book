#include <smmintrin.h>

#include <benchmark/benchmark.h>

#include <boost/unordered/unordered_flat_map.hpp>

#include "itch_01/order_book.hpp"

namespace order_book::itch_01 {
static void BM_itch_01_bid_add_delete(benchmark::State &state) {
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

//BENCHMARK(BM_itch_01_bid_add_delete);
} // order_book::itch_01

static void BM_boost_unordered_flat_map(benchmark::State &state) {
//   printf("%lu\n", sizeof(order_book::itch_01::Foo));

   uint64_t p1 = std::rand();
   boost::unordered_flat_map<uint64_t, uint64_t> map{};

   for (auto _: state) {
      const auto key = p1++ & 32'767U;
      if (const auto e = map.find(key); e == map.end()) {
         map[key] = 1;
      }
      else {
         ++e->second;
      }
      benchmark::ClobberMemory();
   }

   uint64_t tot = 0;
   for (auto &[k, v]: map) {
      tot += v;
   }

   benchmark::DoNotOptimize(++tot);
}

static void BM_div_32(benchmark::State &state) {
   int32_t p1 = std::rand();
   int32_t r1 = 0;

   for (auto _: state) {
      constexpr int32_t div = 100;
      ++p1;
      benchmark::DoNotOptimize(r1 = p1 / div - p1);
      benchmark::ClobberMemory();
   }

   benchmark::DoNotOptimize(++r1);
}

static void BM_hash(benchmark::State &state) {
   uint32_t x = std::rand();

   for (auto _: state) {
      ++x;
      x ^= x >> 16;
      x *= 0x21f0aaadU;
      x ^= x >> 15;
      x *= 0xd35a2d97U;
      x ^= x >> 15;
      x &= 1'048'575;
   }

   benchmark::DoNotOptimize(x);
}

static inline uint32_t crc32c(uint32_t x, uint32_t k) { return _mm_crc32_u32(x, k); }

static void BM_crc_hash(benchmark::State &state) {
   uint32_t x = std::rand();

   for (auto _: state) {
      ++x;
      x = crc32c(x, 0x7cdff266U);
      x *= 0x9c80bf99U;
      x = crc32c(x, 0xf789c7a9U);
      x *= 0x0c9cb5b5U;
      x &= 1'048'575;
   }

   benchmark::DoNotOptimize(x);
}

BENCHMARK(BM_boost_unordered_flat_map);
BENCHMARK(BM_div_32);
BENCHMARK(BM_hash);
BENCHMARK(BM_crc_hash);
