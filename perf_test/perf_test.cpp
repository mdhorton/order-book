#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <chrono>
#include <map>

#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>

#include "nostromo/mmap.hpp"
#include "nostromo/time_utils.hpp"

#include "itch/itch.hpp"
#include "itch/v02/order_book.hpp"

namespace order_book::itch::v02::perf_test {

class PerfTest {
public:
   static auto CreateMetaData(
         unsigned char *data,
         const uint64_t fsize) {
      // stock_code -> set<price>
      MAP<uint16_t, SET<uint32_t>> stock_prices;
      uint32_t max_order_id = 0;
      uint64_t offset = 0;

      while (offset < fsize) {
         auto msg_type = data[offset++];

         switch (msg_type) {
            case 'A':
            case 'F': {
               auto order = (ItchOrderAdd *) &data[offset];
               if (order->order_id > max_order_id) max_order_id = order->order_id;
               stock_prices[order->stock_code].insert(order->price);
               offset += 23;
               break;
            }
            case 'E':
            case 'C':
            case 'X':
               offset += 18;
               break;
            case 'D':
               offset += 14;
               break;
            case 'U': {
               auto order = (ItchOrderReplace *) &data[offset];
               if (order->new_order_id > max_order_id) max_order_id = order->new_order_id;
               stock_prices[order->stock_code].insert(order->price);
               offset += 26;
               break;
            }
            default:
               throw std::runtime_error("unexpected msg_type at offset: " + std::to_string(offset - 1));
         }
      }

      return std::make_pair(max_order_id, stock_prices);
   }
};

} // namespace order_book::itch::v02::perf_test

auto count_orders(
      unsigned char *data,
      const uint64_t fsize) {

}

int main() {
   using order_book::itch::v02::perf_test::PerfTest;

   const std::string base_dir = "/remote/data/nasdaq-itch/";
   const std::string fpath = base_dir + "01302020.NASDAQ_ITCH50.bin";
//   const std::string fpath = base_dir + "12302019.NASDAQ_ITCH50.bin";

   nostromo::Mmap<unsigned char> mmap{fpath};
   auto data = mmap.Ptr();
   auto fsize = mmap.Size();

   auto start = nostromo::TimeUtils::Now();
   auto [max_order_id, stock_prices] = PerfTest::CreateMetaData(data, fsize);
   auto stop = nostromo::TimeUtils::Now();
   auto elap = stop - start;

   printf("max order id: %u\n", max_order_id);
   printf("elapsed: %zu\n", elap.count());

   return 0;
}
