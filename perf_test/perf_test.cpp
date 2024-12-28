#include <cstdint>
#include <stdexcept>
#include <string>
#include <iostream>

#include "nostromo/mmap.hpp"
#include "nostromo/time_utils.hpp"
#include "nostromo/thread_utils.hpp"

#include "itch/itch.hpp"
#include "itch/metadata_io.hpp"
#include "itch/v02/order_book.hpp"
#include "itch/v03/order_book.hpp"
#include "itch/v04/order_book.hpp"

namespace order_book::itch::perf_test {

class PerfTest {
private:
   template<typename BOOKS>
   static void RunPerfTest(std::string &fpath, size_t page_size = 0) {
      auto start = nostromo::TimeUtils::Now();

      auto [max_order_id, max_stock_code, stock_prices]
            = MetadataIO::Read(fpath);

      BOOKS books{max_order_id, max_stock_code, stock_prices, page_size};

      nostromo::Mmap<char> mmap{fpath};
      auto data = mmap.Span();

      uint64_t offset = 0;
      uint64_t order_cnt = 0;

      while (offset < data.size()) {
         ++order_cnt;
         auto msg_type = data[offset++];

         switch (msg_type) {
            case 'A':
            case 'F': {
               auto order = reinterpret_cast<ItchOrderAdd *>(&data[offset]);
               books.OrderAdd(*order);
               offset += 23;
               break;
            }
            case 'E':
            case 'C': {
               auto order = reinterpret_cast<ItchOrderExecuted *>(&data[offset]);
               books.OrderExecuted(*order);
               offset += 18;
               break;
            }
            case 'X': {
               auto order = reinterpret_cast<ItchOrderCancel *>(&data[offset]);
               books.OrderCancel(*order);
               offset += 18;
               break;
            }
            case 'D': {
               auto order = reinterpret_cast<ItchOrderDelete *>(&data[offset]);
               books.OrderDelete(*order);
               offset += 14;
               break;
            }
            case 'U': {
               auto order = reinterpret_cast<ItchOrderReplace *>(&data[offset]);
               books.OrderReplace(*order);
               offset += 26;
               break;
            }
            default:
               throw std::runtime_error(
                     "unexpected msg_type at offset: " +
                     std::to_string(offset - 1));
         }
      }

      auto stop = nostromo::TimeUtils::Now();
      auto elap = stop - start;
      auto ops = static_cast<uint64_t>(
            static_cast<double>(order_cnt) /
            (static_cast<double>(elap.count()) / 1'000'000'000));
      auto npo = elap.count() / order_cnt;

      std::cout
            << "RunPerfTest" << std::endl
            << "order count: " << order_cnt << std::endl
            << "elapsed: " << elap.count() << std::endl
            << "orders/sec: " << ops << std::endl
            << "nanos/order: " << npo << std::endl
            << std::endl;
   }

public:
   static void Run(std::string &fpath) {
      std::cout << "processing: " << fpath << std::endl;
//      RunPerfTest<v02::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_2MB);
//      RunPerfTest<v03::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
      v04::Foo::Bar();
   }
};

} // namespace order_book::itch::perf_test

int main() {
   nostromo::ThreadUtils::SetAffinity(23);

   std::cout.imbue(std::locale(""));
   std::string base_dir = "/remote/data/nasdaq-itch/";

   auto fnames = {
//         "01302019.NASDAQ_ITCH50.sorted-bin",
//         "01302020.NASDAQ_ITCH50.sorted-bin",
         "12302019.NASDAQ_ITCH50.sorted-bin"
   };

   for (auto &fname: fnames) {
      auto fpath = base_dir + fname;
      order_book::itch::perf_test::PerfTest::Run(fpath);
   }

   return 0;
}
