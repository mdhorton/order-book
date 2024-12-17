#include <cstdint>
#include <stdexcept>
#include <string>
#include <iostream>
#include <set>

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
         char *data,
         const uint64_t fsize) {
      MAP<uint32_t, uint8_t> bid_map;
      // stock_code -> pair<bid_prices, ask_prices>
      MAP<uint16_t, std::pair<SET<uint32_t>, SET<uint32_t>>> stock_prices;
      uint32_t max_order_id = 0;
      uint64_t offset = 0;

      while (offset < fsize) {
         auto msg_type = data[offset++];

         switch (msg_type) {
            case 'A':
            case 'F': {
               auto order = (ItchOrderAdd *) &data[offset];
               if (order->order_id > max_order_id) max_order_id = order->order_id;
               if (order->bid) stock_prices[order->stock_code].first.insert(order->price);
               else stock_prices[order->stock_code].second.insert(order->price);
               bid_map[order->order_id] = order->bid;
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
               if (bid_map[order->order_id]) stock_prices[order->stock_code].first.insert(order->price);
               else stock_prices[order->stock_code].second.insert(order->price);
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

int main() {
   using order_book::itch::v02::perf_test::PerfTest;

   std::cout.imbue(std::locale(""));
   const std::string base_dir = "/remote/data/nasdaq-itch/";

   const auto fnames = {
         "01302019.NASDAQ_ITCH50.bin",
         "01302020.NASDAQ_ITCH50.bin",
         "12302019.NASDAQ_ITCH50.bin"
   };

   for (const auto &fname: fnames) {
      const auto fpath = base_dir + fname;
      std::cout << "processing: " << fpath << std::endl;

      nostromo::Mmap<char> mmap{fpath};
      auto data = mmap.Ptr();
      auto fsize = mmap.Size();

      auto start = nostromo::TimeUtils::Now();
      auto [max_order_id, stock_prices] =
            PerfTest::CreateMetaData(data, fsize);
      auto stop = nostromo::TimeUtils::Now();
      auto elap = stop - start;

      std::cout
            << "max order id: " << max_order_id << std::endl
            << "elapsed: " << elap.count() << std::endl;
      std::cout << std::endl;
   }

   return 0;
}
