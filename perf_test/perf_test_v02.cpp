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

namespace order_book::itch::perf_test {
class PerfTest {
   template<typename BOOKS>
   static void RunPerfTest(std::string &fpath, size_t page_size = 0) {
      const auto start = nostromo::TimeUtils::Now();

      using PROC = uint64_t(BOOKS &, unsigned char *);

      auto proc_add = [](BOOKS &books, unsigned char *data) -> uint64_t {
         const auto order = reinterpret_cast<ItchOrderAdd *>(data);
         books.OrderAdd(*order);
         return 23;
      };

      auto proc_executed = [](BOOKS &books, unsigned char *data) -> uint64_t {
         const auto order = reinterpret_cast<ItchOrderExecuted *>(data);
         books.OrderExecuted(*order);
         return 18;
      };

      auto proc_cancel = [](BOOKS &books, unsigned char *data) -> uint64_t {
         const auto order = reinterpret_cast<ItchOrderCancel *>(data);
         books.OrderCancel(*order);
         return 18;
      };

      auto proc_delete = [](BOOKS &books, unsigned char *data) -> uint64_t {
         const auto order = reinterpret_cast<ItchOrderDelete *>(data);
         books.OrderDelete(*order);
         return 14;
      };

      auto proc_replace = [](BOOKS &books, unsigned char *data) -> uint64_t {
         const auto order = reinterpret_cast<ItchOrderReplace *>(data);
         books.OrderReplace(*order);
         return 26;
      };

      auto proc_error = []([[maybe_unused]] BOOKS &books, [[maybe_unused]] unsigned char *data) -> uint64_t {
         throw std::runtime_error("unexpected msg_type");
      };

      std::vector<PROC *> processors(255, proc_error);
      processors[65] = proc_add; // A
      processors[70] = proc_add; // F
      processors[69] = proc_executed; // E
      processors[67] = proc_executed; // C
      processors[88] = proc_cancel; // X
      processors[68] = proc_delete; // D
      processors[85] = proc_replace; // U

      const auto [max_order_id, max_stock_code, stock_prices]
            = MetadataIO::Read(fpath);

      BOOKS books{max_order_id, max_stock_code, stock_prices, page_size};

      uint64_t offset = 0;
      uint64_t order_cnt = 0;

      const nostromo::Mmap<unsigned char> mmap{fpath};
      const auto data = mmap.Span();

      while (offset < data.size()) {
         const auto msg_type = data[offset];
         ++offset;
         const auto processor = processors[msg_type];
         offset += processor(books, &data[offset]);
         ++order_cnt;
      }

      const auto stop = nostromo::TimeUtils::Now();
      const auto elap = stop - start;
      const auto order_cnt_d = static_cast<double>(order_cnt);
      const auto elap_d = static_cast<double>(elap.count());
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
   static void Run(std::string &fpath) {
      std::cout << "processing: " << fpath << std::endl;
      // RunPerfTest<v02::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
      // RunPerfTest<v03::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
      // RunPerfTest<v05::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
      // RunPerfTest<v06::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
      // RunPerfTest<v07::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
      // RunPerfTest<v08::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
      RunPerfTest<v09::OrderBooks>(fpath, nostromo::HugePageUtils::SIZE_1GB);
   }
};
} // namespace order_book::itch::perf_test

int main() {
   nostromo::ThreadUtils::SetAffinity(11);

   std::cout.imbue(std::locale(""));
   const std::string base_dir = "/remote/data/nasdaq-itch/";

   const auto fnames = {
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
