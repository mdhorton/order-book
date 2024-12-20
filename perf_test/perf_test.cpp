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
   static auto RunPerfTest(
         char *data,
         uint64_t fsize,
         uint32_t max_order_id,
         std::map<uint16_t, std::pair<std::set<uint32_t>, std::set<uint32_t>>> &stock_prices) {
      auto start = nostromo::TimeUtils::Now();

      OrderBooks books{max_order_id, stock_prices};

      uint64_t offset = 0;
      uint64_t order_cnt = 0;

      while (offset < fsize) {
         ++order_cnt;
//         if (order_cnt > 10'000'000) break;

         auto msg_type = data[offset++];

         switch (msg_type) {
            case 'A':
            case 'F': {
               auto order = (ItchOrderAdd *) &data[offset];
               books.OrderAdd(*order);
               offset += 23;
               break;
            }
            case 'E':
            case 'C': {
               auto order = (ItchOrderExecuted *) &data[offset];
               books.OrderExecuted(*order);
               offset += 18;
               break;
            }
            case 'X': {
               auto order = (ItchOrderCancel *) &data[offset];
               books.OrderCancel(*order);
               offset += 18;
               break;
            }
            case 'D': {
               auto order = (ItchOrderDelete *) &data[offset];
               books.OrderDelete(*order);
               offset += 14;
               break;
            }
            case 'U': {
               auto order = (ItchOrderReplace *) &data[offset];
               books.OrderReplace(*order);
               offset += 26;
               break;
            }
            default:
               throw std::runtime_error("unexpected msg_type at offset: " + std::to_string(offset - 1));
         }
      }

      auto stop = nostromo::TimeUtils::Now();
      auto elap = stop - start;
      auto ops = order_cnt / (elap.count() / 1'000'000'000);

      std::cout
            << "RunPerfTest" << std::endl
            << "order count: " << order_cnt << std::endl
            << "elapsed: " << elap.count() << std::endl
            << "orders/sec: " << ops << std::endl
            << std::endl;
   }

   static auto Run(const std::string &fpath) {
      std::cout << "processing: " << fpath << std::endl;

      nostromo::Mmap<char> mmap{fpath};
      auto data = mmap.Ptr();
      auto fsize = mmap.Size();

//      auto [max_order_id, stock_prices] = CreateMetaData(data, fsize);
//      auto stock_prices_sorted = SortMetaData(stock_prices);
//      RunPerfTest(data, fsize, max_order_id, stock_prices_sorted);
   }
};

} // namespace order_book::itch::v02::perf_test

int main() {
   using order_book::itch::v02::perf_test::PerfTest;

   std::cout.imbue(std::locale(""));
   const std::string base_dir = "/remote/data/nasdaq-itch/";

   const auto fnames = {
//         "01302019.NASDAQ_ITCH50.bin",
//         "01302020.NASDAQ_ITCH50.bin",
         "12302019.NASDAQ_ITCH50.bin"
   };

   for (const auto &fname: fnames) {
      const auto fpath = base_dir + fname;
      PerfTest::Run(fpath);
   }

   return 0;
}
