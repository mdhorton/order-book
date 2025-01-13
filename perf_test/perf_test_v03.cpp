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
#include "itch/v13/order_book.hpp"

#include "nostromo/mmap.hpp"
#include "nostromo/time_utils.hpp"
#include "nostromo/huge_page.hpp"
#include "nostromo/thread_utils.hpp"

#include <fmt/format.h>

#include <cstdint>
#include <stdexcept>
#include <string>

namespace order_book::itch::perf_test {

class PerfTest {
   template<typename BOOKS, typename ADD, typename REPLACE>
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
               const auto order = reinterpret_cast<ADD *>(&data[offset]);
               books.OrderAdd(*order);
               offset += sizeof(ADD);
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
               const auto order = reinterpret_cast<REPLACE *>(&data[offset]);
               books.OrderReplace(*order);
               offset += sizeof(REPLACE);
               break;
            }
            default:
               throw std::runtime_error("unsupported msg_type at offset: " + std::to_string(offset - 1));
         }
      }

      const auto elap = nostromo::TimeUtils::Now() - start;
      const auto ops = Utils::Ops(elap.count(), order_cnt);
      const auto npo = static_cast<double>(elap.count()) / static_cast<double>(order_cnt);

      fmt::print("{}", fmt::format(
            std::locale("en_US.UTF-8"),
            "order count: {:L}\n"
            "elapsed: {:L} ns\n"
            "orders/sec: {:L}\n"
            "nanos/order: {:.2f}\n\n",
            order_cnt, elap.count(), ops, npo
      ));
   }

public:
   static void Run(const std::string &fpath) {
      fmt::print("processing: {}\n", fpath);

      const auto page_sizes = {nostromo::HugePage::SIZE_1GB};
      for (const auto page_size: page_sizes) {
//         RunPerfTest<v02::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
//         RunPerfTest<v03::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
//         RunPerfTest<v05::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
//         RunPerfTest<v06::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
//         RunPerfTest<v07::OrderBooks, ItchOrderAdd, ItchOrderReplace>(fpath, page_size);
         RunPerfTest<v08::OrderBooks, ItchOrderAdd, ItchOrderReplace>(fpath + "-sorted", page_size);
         RunPerfTest<v09::OrderBooks, ItchOrderAdd, ItchOrderReplace>(fpath + "-sorted", page_size);
         RunPerfTest<v10::OrderBooks, ItchOrderAdd, ItchOrderReplace>(fpath + "-sorted", page_size);
         RunPerfTest<v11::OrderBooks, ItchOrderAdd, ItchOrderReplace>(fpath + "-sorted", page_size);
         RunPerfTest<v12::OrderBooks, ItchOrderAddIdx, ItchOrderReplaceIdx>(fpath + "-sorted-idx", page_size);
         RunPerfTest<v13::OrderBooks, ItchOrderAddIdx, ItchOrderReplaceIdx>(fpath + "-sorted-idx", page_size);
      }
   }
};

} // namespace order_book::itch::perf_test

int main() {
   nostromo::ThreadUtils::SetAffinity(11);
   namespace itch = order_book::itch;

   for (const auto &fname: itch::DATA_FILE_NAMES) {
      const auto fpath = itch::DATA_DIR_BASE + fname + ".bin";
      order_book::itch::perf_test::PerfTest::Run(fpath);
   }

   return 0;
}
