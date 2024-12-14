#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <chrono>

#include <boost/unordered/unordered_flat_map.hpp>

#include <itch_01/order_book.hpp>

struct Counts {
   uint32_t order_count{};
   boost::unordered_flat_map<uint32_t, uint32_t> price_counts{};
};

void update_hilo(
      boost::unordered_flat_map<uint16_t, Counts> &map,
      const uint16_t stock_code,
      const uint32_t price) {
   if (const auto stock_val = map.find(stock_code); stock_val == map.end()) {
      map[stock_code] = Counts{.order_count=1};
   }
   else {
      auto &counts = stock_val->second;
      ++counts.order_count;

      if (price > 0) {
         auto &price_counts = counts.price_counts;
         if (const auto price_val = price_counts.find(price); price_val == price_counts.end()) {
            price_counts[price] = 1;
         }
         else {
            ++price_val->second;
         }
      }
   }
}

auto hilo_prices(unsigned char *ptr, const uint64_t fsize) {
   boost::unordered_flat_map<uint16_t, Counts> map{};
   uint64_t offset = 0;

   while (offset < fsize) {
      switch (ptr[offset++]) {
         case 'A':
         case 'F': {
            const auto order = reinterpret_cast<order_book::ItchOrderAdd *>(&ptr[offset]);
            update_hilo(map, order->stock_code, order->price);
            offset += 23;
            break;
         }
         case 'E':
         case 'C': {
            const auto order = reinterpret_cast<order_book::ItchOrderExecuted *>(&ptr[offset]);
            update_hilo(map, order->stock_code, 0);
            offset += 18;
            break;
         }
         case 'X': {
            const auto order = reinterpret_cast<order_book::ItchOrderCancel *>(&ptr[offset]);
            update_hilo(map, order->stock_code, 0);
            offset += 18;
            break;
         }
         case 'D': {
            const auto order = reinterpret_cast<order_book::ItchOrderDelete *>(&ptr[offset]);
            update_hilo(map, order->stock_code, 0);
            offset += 14;
            break;
         }
         case 'U': {
            const auto order = reinterpret_cast<order_book::ItchOrderReplace *>(&ptr[offset]);
            update_hilo(map, order->stock_code, order->price);
            offset += 26;
            break;
         }
         default:
            throw std::runtime_error("unexpected msg_type at offset: " + std::to_string(offset - 1));
      }
   }

   return map;
}

int main() {
   const std::string base_dir = "/remote/data/nasdaq-itch/";
   const std::string data_file = base_dir + "12302019.NASDAQ_ITCH50.bin";

   errno = 0;
   const auto fd = ::open(data_file.c_str(), O_RDONLY);
   if (fd == -1) {
      printf("open() failed: %d\n", errno);
      return -1;
   }

   struct stat st{};

   errno = 0;
   if (::fstat(fd, &st) == -1) {
      printf("fstat() failed: %d\n", errno);
      return -1;
   }

   const auto fsize = st.st_size;
   constexpr int flags = MAP_PRIVATE | MAP_POPULATE;

   errno = 0;
   const auto data = ::mmap(nullptr, fsize, PROT_READ, flags, fd, 0);
   if (data == MAP_FAILED) {
      printf("mmap() failed: %d\n", errno);
      return -1;
   }

   const auto ptr = static_cast<unsigned char *>(data);
   using Clock = std::conditional<
         std::chrono::high_resolution_clock::is_steady,
         std::chrono::high_resolution_clock,
         std::chrono::steady_clock>::type;

   auto start = Clock::now();
   auto map = hilo_prices(ptr, fsize);
   auto stop = Clock::now();
   auto diff = stop - start;

   for (auto &[key, val]: map) {
      printf("stock: %d -> count: %d (", key, val.order_count);
      for (auto &[k, v]: val.price_counts) {
         printf("%d -> %d  ", k, v);
      }
      printf(")\n");
   }

   printf("stocks: %zu\n", map.size());
   printf("elapsed: %zu\n", diff.count());
//  printf("%zu\n", diff.count() / total);

   // divisor 1 if high < 10000 else 100
   // offset = low / divisor
//  order_book::itch_01::OrderBook book{100, 2500};
//
//  uint64_t offset = 0;

//  while (offset < fsize) {
//    switch (ptr[offset++]) {
//      case 'A':
//      case 'F': {
//        book.OrderAdd(reinterpret_cast<order_book::ItchOrderAdd *>(&ptr[offset]));
//        offset += 23;
//        break;
//      }
//      case 'E':
//      case 'C':
//        // book.OrderExecuted(*reinterpret_cast<order_book::ItchOrderExecuted *>(ptr[offset]));
//        offset += 18;
//        break;
//      case 'X':
//        // book.OrderCancel(*reinterpret_cast<order_book::ItchOrderCancel *>(ptr[offset]));
//        offset += 18;
//        break;
//      case 'D':
//        // book.OrderDelete(*reinterpret_cast<order_book::ItchOrderDelete *>(ptr[offset]));
//        offset += 14;
//        break;
//      case 'U':
//        // book.OrderReplace(*reinterpret_cast<order_book::ItchOrderReplace *>(ptr[offset]));
//        offset += 26;
//        break;
//      default:
//        throw std::runtime_error("unexpected msg_type at offset: " + std::to_string(offset - 1));
//    }
//  }

   ::munmap(data, fsize);
   ::close(fd);

   return 0;
}
