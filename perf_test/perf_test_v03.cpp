#include "itch/itch.hpp"
#include "itch/metadata_io.hpp"

//#include "itch/v03/order_book.hpp"
//#include "itch/v05/order_book.hpp"
//#include "itch/v06/order_book.hpp"
//#include "itch/v07/order_book.hpp"
//#include "itch/v08/order_book.hpp"
//#include "itch/v09/order_book.hpp"
//#include "itch/v10/order_book.hpp"
//#include "itch/v11/order_book.hpp"
#include "itch/v12/order_book.hpp"
#include "itch/v13/order_book.hpp"
#include "itch/v14/order_book.hpp"
#include "itch/v15/order_book.hpp"
#include "itch/v16/order_book.hpp"
#include "itch/v17/order_book.hpp"
#include "itch/v18/order_book.hpp"

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
   const std::string &fprefix_;

public:
   explicit PerfTest(const std::string &fprefix)
         : fprefix_{fprefix} {}

   template<typename BOOKS, typename ADD, typename REPLACE>
   void Execute(
         const std::string &test_id,
         const std::string &meta_suffix,
         const std::string &bin_suffix,
         const size_t orders_page_size = nostromo::HugePage::SIZE_1GB,
         const size_t other_page_size = nostromo::HugePage::SIZE_2MB) {
      const auto start = nostromo::TimeUtils::Now();

      const auto [
            max_order_id,
            max_stock_code,
            stock_price_map
      ] = MetadataIO::Read(fprefix_ + ".meta" + meta_suffix);

      BOOKS books{max_order_id, max_stock_code, stock_price_map, orders_page_size, other_page_size};

      const nostromo::Mmap<char> mmap{fprefix_ + ".bin" + bin_suffix};
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
      const auto npo = static_cast<double>(elap.count()) / static_cast<double>(order_cnt);

      fmt::print("test id: {}  ns/order: {:.4f}\n", test_id, npo);
   }

public:
   void Run() {
      const auto sorted = std::string{"-sorted"};
      const auto reverse_bid = std::string{"-reverse-bid"};
      const auto reverse_ask = std::string{"-reverse-ask"};
      const auto sorted_idx = sorted + "-idx";
      const auto sorted_idx_rbid = sorted_idx + reverse_bid;
      const auto sorted_idx_rask = sorted_idx + reverse_ask;

//      const auto orders_page_sizes = {nostromo::HugePage::SIZE_2MB, nostromo::HugePage::SIZE_1GB};
//      const auto other_page_sizes = {nostromo::HugePage::SIZE_2MB, nostromo::HugePage::SIZE_1GB};
//
//      for (const auto orders_page_size: orders_page_sizes) {
//         for (const auto other_page_size: other_page_sizes) {
//         }
//      }
//         RunPerfTest<v03::OrderBooks, ItchOrderAdd, ItchOrderReplace>("03", sorted, page_size);
//         RunPerfTest<v05::OrderBooks, ItchOrderAdd, ItchOrderReplace>("05", sorted, page_size);
//         RunPerfTest<v06::OrderBooks, ItchOrderAdd, ItchOrderReplace>("06", sorted, page_size);
//         RunPerfTest<v07::OrderBooks, ItchOrderAdd, ItchOrderReplace>("07", sorted, page_size);
//         RunPerfTest<v08::OrderBooks, ItchOrderAdd, ItchOrderReplace>("08", sorted, page_size);
//         RunPerfTest<v09::OrderBooks, ItchOrderAdd, ItchOrderReplace>("09", sorted, page_size);
//         RunPerfTest<v10::OrderBooks, ItchOrderAdd, ItchOrderReplace>("10", sorted, page_size);
//         RunPerfTest<v11::OrderBooks, ItchOrderAdd, ItchOrderReplace>("11", sorted, page_size);

      Execute<v12::OrderBooks, ItchOrderAddIdx, ItchOrderReplaceIdx>("12", "", sorted_idx);
      Execute<v12::OrderBooks, ItchOrderAddIdx, ItchOrderReplaceIdx>("12", "", sorted_idx);

      Execute<v13::OrderBooks, ItchOrderAddIdx, ItchOrderReplaceIdx>("13", "", sorted_idx);
      Execute<v13::OrderBooks, ItchOrderAddIdx, ItchOrderReplaceIdx>("13", "", sorted_idx);

      Execute<v14::OrderBooks, ItchOrderAddIdx, ItchOrderReplaceIdx>("14", "", sorted_idx);
      Execute<v14::OrderBooks, ItchOrderAddIdx, ItchOrderReplaceIdx>("14", "", sorted_idx);

      Execute<v15::OrderBooks, ItchOrderAddIdx, ItchOrderReplaceIdx>("15", "", sorted_idx);
      Execute<v15::OrderBooks, ItchOrderAddIdx, ItchOrderReplaceIdx>("15", "", sorted_idx);

      Execute<v16::OrderBooks, ItchOrderAddIdx, ItchOrderReplaceIdx>("16", "", sorted_idx);
      Execute<v16::OrderBooks, ItchOrderAddIdx, ItchOrderReplaceIdx>("16", "", sorted_idx);

      Execute<v17::OrderBooks, ItchOrderAddIdx, ItchOrderReplaceIdx>("17", reverse_bid, sorted_idx_rbid);
      Execute<v17::OrderBooks, ItchOrderAddIdx, ItchOrderReplaceIdx>("17", reverse_bid, sorted_idx_rbid);

      Execute<v18::OrderBooks, ItchOrderAddIdx, ItchOrderReplaceIdx>("18", reverse_ask, sorted_idx_rask);
      Execute<v18::OrderBooks, ItchOrderAddIdx, ItchOrderReplaceIdx>("18", reverse_ask, sorted_idx_rask);
   }
};

} // namespace order_book::itch::perf_test

int main() {
   const auto cpuid = static_cast<int>(std::thread::hardware_concurrency()) - 1;
   fmt::print("using cpuid: {}\n", cpuid);
   nostromo::ThreadUtils::SetAffinity(cpuid);

   namespace itch = order_book::itch;

   for (const auto &fname: itch::DATA_FILE_NAMES) {
      const auto fprefix = itch::DATA_DIR_BASE + fname;
      fmt::print("processing: {}\n", fprefix);
      itch::perf_test::PerfTest{fprefix}.Run();
   }

   return 0;
}
