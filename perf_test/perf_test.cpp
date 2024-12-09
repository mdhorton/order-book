#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <itch_1/order_book.hpp>

void bar(
   std::unordered_map<uint16_t, std::pair<uint32_t, uint32_t> > &map,
   const uint16_t stock_code,
   const uint32_t price) {
   if (const auto val = map.find(stock_code); val == map.end()) {
      map[stock_code] = std::make_pair(price, price);
   }
   else {
      if (price > val->second.first) val->second.first = price;
      if (price < val->second.second) val->second.second = price;
   }
}

auto foo(void *data, const uint64_t fsize) {
   std::unordered_map<uint16_t, std::pair<uint32_t, uint32_t> > map{};
   const auto ptr = static_cast<char *>(data);
   uint64_t offset = 0;

   while (offset < fsize) {
      switch (ptr[offset++]) {
         case 'A':
         case 'F': {
            const auto order = *reinterpret_cast<order_book::ItchOrderAdd *>(ptr[offset]);
            bar(map, order.stock_code, order.price);
            offset += 23;
            break;
         }
         case 'E':
            offset += 18;
            break;
         case 'C':
            offset += 23;
            break;
         case 'X':
            offset += 18;
            break;
         case 'D':
            offset += 14;
            break;
         case 'U': {
            const auto order = *reinterpret_cast<order_book::ItchOrderReplace *>(ptr[offset]);
            bar(map, order.stock_code, order.price);
            offset += 26;
            break;
         }
         default:
            throw std::runtime_error("unexpected msg_type at offset: " + std::to_string(offset));
      }

      return map;
   }
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

   // divisor 1 if high < 10000 else 100
   // offset = low / divisor
   order_book::itch_1::OrderBook book{100, 2500};

   const auto ptr = static_cast<char *>(data);
   uint64_t offset = 0;

   while (offset < fsize) {
      switch (ptr[offset++]) {
         case 'A':
         case 'F':
            // book.OrderAdd(*reinterpret_cast<order_book::ItchOrderAdd *>(ptr[offset]));
            offset += 23;
            break;
         case 'E':
            // book.OrderExecuted(*reinterpret_cast<order_book::ItchOrderExecuted *>(ptr[offset]));
            offset += 18;
            break;
         case 'C':
            // book.OrderExecutedPrice(*reinterpret_cast<order_book::ItchOrderExecutedPrice *>(ptr[offset]));
            offset += 23;
            break;
         case 'X':
            // book.OrderCancel(*reinterpret_cast<order_book::ItchOrderCancel *>(ptr[offset]));
            offset += 18;
            break;
         case 'D':
            // book.OrderDelete(*reinterpret_cast<order_book::ItchOrderDelete *>(ptr[offset]));
            offset += 14;
            break;
         case 'U':
            // book.OrderReplace(*reinterpret_cast<order_book::ItchOrderReplace *>(ptr[offset]));
            offset += 26;
            break;
         default:
            throw std::runtime_error("unexpected msg_type at offset: " + std::to_string(offset));
      }
   }

   ::munmap(data, fsize);
   ::close(fd);

   return 0;
}
