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
#include "itch/v05/order_book.hpp"
#include "itch/v06/order_book.hpp"
#include "itch/v07/order_book.hpp"
#include "itch/v08/order_book.hpp"
#include "itch/v09/order_book.hpp"
#include "itch/v10/order_book.hpp"
#include "itch/v11/order_book.hpp"
#include "itch/v12/order_book.hpp"

namespace order_book::itch::perf_test {

class PerfTest {
   template<typename BOOKS>
   static void RunPerfTest(const std::string &fpath, const size_t page_size = 0) {
      const auto start = nostromo::TimeUtils::Now();

      const auto [
            max_order_id,
            max_stock_code,
            stock_prices
      ] = MetadataIO::Read(fpath);

      BOOKS books{max_order_id, max_stock_code, stock_prices, page_size};

      const nostromo::Mmap<char> mmap{fpath};
      const auto data = mmap.Span();

      uint64_t offset = 0;
      uint64_t order_cnt = 0;

      while (offset < data.size()) {
         const auto msg_type = data[offset];
         ++offset;
         ++order_cnt;

         switch (msg_type) {
            case 'A':
            case 'F': {
               const auto order = reinterpret_cast<ItchOrderAddEnhanced *>(&data[offset]);
               books.OrderAdd(*order);
               offset += sizeof(ItchOrderAddEnhanced);
               break;
            }
            case 'E':
            case 'C': {
               const auto order = reinterpret_cast<ItchOrderExecuted *>(&data[offset]);
               books.OrderExecuted(*order);
               offset += sizeof(ItchOrderExecuted);
               break;
            }
            case 'X': {
               const auto order = reinterpret_cast<ItchOrderCancel *>(&data[offset]);
               books.OrderCancel(*order);
               offset += sizeof(ItchOrderCancel);
               break;
            }
            case 'D': {
               const auto order = reinterpret_cast<ItchOrderDelete *>(&data[offset]);
               books.OrderDelete(*order);
               offset += sizeof(ItchOrderDelete);
               break;
            }
            case 'U': {
               const auto order = reinterpret_cast<ItchOrderReplaceEnhanced *>(&data[offset]);
               books.OrderReplace(*order);
               offset += sizeof(ItchOrderReplaceEnhanced);
               break;
            }
            default:
               throw std::runtime_error(
                     "unexpected msg_type at offset: " +
                     std::to_string(offset - 1)
               );
         }
      }

      const auto stop = nostromo::TimeUtils::Now();
      const auto elap = stop - start;
      const auto elap_d = static_cast<double>(elap.count());
      const auto order_cnt_d = static_cast<double>(order_cnt);
      const auto ops = static_cast<uint64_t>(order_cnt_d / (elap_d / 1'000'000'000.0));
      const auto npo = elap_d / order_cnt_d;

      std::cout
            << "RunPerfTest" << std::endl
            << "order count: " << order_cnt << std::endl
            << "elapsed: " << elap.count() << std::endl
            << "orders/sec: " << ops << std::endl
            << "nanos/order: " << npo << std::endl
            << std::endl;
   }

public:
   static void Run(const std::string &fpath) {
      std::cout << "processing: " << fpath << std::endl;
      // RunPerfTest<v02::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
      // RunPerfTest<v03::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
      // RunPerfTest<v05::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
      // RunPerfTest<v06::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
      // RunPerfTest<v07::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
      // RunPerfTest<v08::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
      // RunPerfTest<v09::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
      // RunPerfTest<v10::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
      // RunPerfTest<v10::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
//      RunPerfTest<v11::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
      RunPerfTest<v12::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
      RunPerfTest<v12::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_2MB);
   }
};

} // namespace order_book::itch::perf_test

int main() {
   nostromo::ThreadUtils::SetAffinity(23);

   std::cout.imbue(std::locale(""));
   const std::string base_dir = "/remote/data/nasdaq-itch/";

   const auto fnames = {
         //         "01302019.NASDAQ_ITCH50.sorted-bin",
         //         "01302020.NASDAQ_ITCH50.sorted-bin",
         "12302019.NASDAQ_ITCH50"
   };

   for (const auto &fname: fnames) {
      const auto fpath = base_dir + fname + ".preprocessed";
      order_book::itch::perf_test::PerfTest::Run(fpath);
   }

   return 0;
}
