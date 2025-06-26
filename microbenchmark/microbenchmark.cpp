#include <immintrin.h>

#include <random>
#include <span>

#include <benchmark/benchmark.h>
#include <boost/unordered/unordered_flat_map.hpp>

#include "nostromo/memory_utils.hpp"
#include "nostromo/random.hpp"

static void BM_simd(benchmark::State &state) {
   static auto RNG = nos::Random::MT19937();

   static constexpr int8_t lookup_table[] = {
         0, 0, 0, 0,
         1, 1, 1, 1,
         2, 2, 2, 2,
         3, 3, 3, 3,
         4, 4, 4, 4,
         5, 5, 5, 5,
         6, 6, 6, 6,
         7, 7, 7, 7
   };

   uint32_t vals[8] = {
         RNG(), RNG(), RNG(), RNG(), RNG(), RNG(), RNG(), RNG()
   };

   auto test_val = RNG();
   uint64_t tot = 0;

   for (auto _: state) {
      ++vals[0];
      const auto data = _mm256_load_si256(reinterpret_cast<__m256i *>(vals));
      const auto test = _mm256_set1_epi32(test_val++);
      const auto cmp = _mm256_cmpeq_epi32(test, data);

      if (const auto mask = _mm256_movemask_epi8(cmp); mask != 0) {
         // auto tzcnt = _mm_tzcnt_32(mask);
         // auto tzcnt = __tzcnt_u32(mask);
         // auto idx = lookup_table[tzcnt];
         benchmark::DoNotOptimize(tot += mask);
      }
   }
}

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
   static auto RNG = nos::Random::MT19937_64();

   constexpr auto size = 8u;
   const auto data = nos::MemoryUtils::Calloc<Data>(size);

   for (auto idx = 0u; idx < size; ++idx) {
      data[idx].v1 = RNG();
      data[idx].v2 = RNG();
      data[idx].v3 = RNG();
      data[idx].v4 = RNG();
      data[idx].v5 = RNG();
      data[idx].v6 = RNG();
      data[idx].v7 = RNG();
      data[idx].v8 = RNG();
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
   static auto RNG = nos::Random::MT19937_64();

   constexpr auto size = 8u;
   const auto raw_data = nos::MemoryUtils::Calloc<Data>(size);
   std::span data{raw_data, size};

   for (auto idx = 0u; idx < size; ++idx) {
      data[idx].v1 = RNG();
      data[idx].v2 = RNG();
      data[idx].v3 = RNG();
      data[idx].v4 = RNG();
      data[idx].v5 = RNG();
      data[idx].v6 = RNG();
      data[idx].v7 = RNG();
      data[idx].v8 = RNG();
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
   static auto RNG = nos::Random::MT19937_64();

   uint64_t p1 = RNG();
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
   static auto RNG = nos::Random::MT19937_64();

   uint64_t p1 = RNG();
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
      static auto RNG = nos::Random::MT19937();

   static constexpr int32_t div = 100;
   int32_t p1 = RNG();
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
   static auto RNG = nos::Random::MT19937();

   uint32_t x = RNG();

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

BENCHMARK(BM_simd);
BENCHMARK(BM_ptr);
BENCHMARK(BM_span);
BENCHMARK(BM_std_unordered_map);
BENCHMARK(BM_boost_unordered_flat_map);
BENCHMARK(BM_div_32);
BENCHMARK(BM_hash);
