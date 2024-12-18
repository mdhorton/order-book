#include <span>

#include <benchmark/benchmark.h>

#include <boost/unordered/unordered_flat_map.hpp>

#include "nostromo/memory_utils.hpp"

#include "itch/itch.hpp"

struct Data {
   uint64_t v1;
   uint64_t v2;
   uint64_t v3;
   uint64_t v4;
   uint64_t v5;
   uint64_t v6;
   uint64_t v7;
   uint64_t v8;
};

static void BM_ptr(benchmark::State &state) {
   auto size = 8u;
   auto data = nostromo::MemoryUtils::Calloc<Data>(size);

   for (auto idx = 0u; idx < size; ++idx) {
      data[idx].v1 = std::rand();
      data[idx].v2 = std::rand();
      data[idx].v3 = std::rand();
      data[idx].v4 = std::rand();
      data[idx].v5 = std::rand();
      data[idx].v6 = std::rand();
      data[idx].v7 = std::rand();
      data[idx].v8 = std::rand();
   }

   for (auto _: state) {
      for (auto idx = 0u; idx < size; ++idx) {
         benchmark::DoNotOptimize(++data[idx].v1);
         benchmark::DoNotOptimize(++data[idx].v2);
         benchmark::DoNotOptimize(++data[idx].v3);
         benchmark::DoNotOptimize(++data[idx].v4);
         benchmark::DoNotOptimize(++data[idx].v5);
         benchmark::DoNotOptimize(++data[idx].v6);
         benchmark::DoNotOptimize(++data[idx].v7);
         benchmark::DoNotOptimize(++data[idx].v8);
      }
      benchmark::ClobberMemory();
   }

   state.SetItemsProcessed(size * state.iterations());
   free(data);
}

static void BM_span(benchmark::State &state) {
   auto size = 8u;
   auto raw_data = nostromo::MemoryUtils::Calloc<Data>(size);
   std::span<Data> data{raw_data, size};

   for (auto idx = 0u; idx < size; ++idx) {
      data[idx].v1 = std::rand();
      data[idx].v2 = std::rand();
      data[idx].v3 = std::rand();
      data[idx].v4 = std::rand();
      data[idx].v5 = std::rand();
      data[idx].v6 = std::rand();
      data[idx].v7 = std::rand();
      data[idx].v8 = std::rand();
   }

   for (auto _: state) {
      for (auto idx = 0u; idx < size; ++idx) {
         benchmark::DoNotOptimize(++data[idx].v1);
         benchmark::DoNotOptimize(++data[idx].v2);
         benchmark::DoNotOptimize(++data[idx].v3);
         benchmark::DoNotOptimize(++data[idx].v4);
         benchmark::DoNotOptimize(++data[idx].v5);
         benchmark::DoNotOptimize(++data[idx].v6);
         benchmark::DoNotOptimize(++data[idx].v7);
         benchmark::DoNotOptimize(++data[idx].v8);
      }
      benchmark::ClobberMemory();
   }

   state.SetItemsProcessed(size * state.iterations());
   free(raw_data);
}

static void BM_std_unordered_map(benchmark::State &state) {
   uint64_t p1 = std::rand();
   std::unordered_map<uint64_t, uint64_t> map;

   for (auto _: state) {
      auto key = p1++ & 32'767U;
      benchmark::DoNotOptimize(++map[key]);
   }

   uint64_t tot = 0;
   for (auto &[k, v]: map) {
      tot += v;
   }

   benchmark::DoNotOptimize(++tot);
}

static void BM_boost_unordered_flat_map(benchmark::State &state) {
   uint64_t p1 = std::rand();
   boost::unordered_flat_map<uint64_t, uint64_t> map;

   for (auto _: state) {
      auto key = p1++ & 32'767U;
      benchmark::DoNotOptimize(++map[key]);
   }

   uint64_t tot = 0;
   for (auto &[k, v]: map) {
      tot += v;
   }

   benchmark::DoNotOptimize(++tot);
}

static void BM_div_32(benchmark::State &state) {
   static constexpr int32_t div = 100;
   int32_t p1 = std::rand();
   int32_t r1 = 0;

   for (auto _: state) {
      ++p1;
      benchmark::DoNotOptimize(r1 = p1 / div - p1);
      benchmark::ClobberMemory();
   }

   benchmark::DoNotOptimize(++r1);
}

// calculate a decent quality 32-bit hash.
// https://github.com/skeeto/hash-prospector
static void BM_hash(benchmark::State &state) {
   uint32_t x = std::rand();

   for (auto _: state) {
      ++x;
      x ^= x >> 16;
      x *= 0x7feb352d;
      x ^= x >> 15;
      x *= 0x846ca68b;
      x ^= x >> 16;
   }

   benchmark::DoNotOptimize(x);
}

BENCHMARK(BM_ptr);
BENCHMARK(BM_span);
BENCHMARK(BM_std_unordered_map);
BENCHMARK(BM_boost_unordered_flat_map);
BENCHMARK(BM_div_32);
BENCHMARK(BM_hash);
