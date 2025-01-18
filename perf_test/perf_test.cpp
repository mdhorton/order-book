#include "itch/common.hpp"
#include "itch/itch.hpp"
#include "itch/metadata_io.hpp"

#include "itch/v11/order_book.hpp"

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
   const std::string &fname_;
   std::ofstream &out_;

public:
   explicit PerfTest(const std::string &fname, std::ofstream &out)
         : fname_{fname},
           out_{out} {}

   template<typename BOOKS>
   void Execute(
         const std::string &test_id,
         const std::string &meta_suffix,
         const std::string &bin_suffix,
         const size_t orders_page_size = nostromo::HugePage::SIZE_1GB,
         const size_t other_page_size = nostromo::HugePage::SIZE_2MB) {
      const auto start = nostromo::TimeUtils::Now();
      const auto fprefix = itch::DATA_DIR_BASE + fname_;

      const auto [
            max_order_id,
            max_stock_code,
            stock_price_map
      ] = MetadataIO::Read(fprefix + ".meta" + meta_suffix);

      BOOKS books{max_order_id, max_stock_code, stock_price_map, orders_page_size, other_page_size};

      const nostromo::Mmap<char> mmap{fprefix + ".bin" + bin_suffix};
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
               const auto order = reinterpret_cast<ItchOrderAdd *>(&data[offset]);
               books.OrderAdd(*order);
               offset += sizeof(ItchOrderAdd);
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
               const auto order = reinterpret_cast<ItchOrderReplace *>(&data[offset]);
               books.OrderReplace(*order);
               offset += sizeof(ItchOrderReplace);
               break;
            }
            default:
               throw std::runtime_error("unsupported msg_type at offset: " + std::to_string(offset - 1));
         }
      }

      const auto elap = nostromo::TimeUtils::Now() - start;
      const auto npo = static_cast<double>(elap.count()) / static_cast<double>(order_cnt);

      out_ << test_id << "," << fname_ << "," << elap.count() << "," << order_cnt << "," << npo << std::endl;
      fmt::print("test id: {}  ns/order: {:.4f}\n", test_id, npo);
   }

public:
   void Run() {
      const auto version = std::string{"v11"};
      const auto meta_suffix = std::string{""};
      const auto bin_suffix = std::string{"-sorted"};
      Execute<v11::OrderBooks>(version, meta_suffix, bin_suffix);
   }
};

} // namespace order_book::itch::perf_test

int main(int argc, char **argv) {
   const auto test_id = argc == 2 ? std::string{argv[1]} : "default";
   const auto out_path = "/tmp/PerfTest-" + test_id + ".csv";
   std::ofstream out(out_path, std::ios_base::out);

   const auto cpuid = static_cast<int>(std::thread::hardware_concurrency()) - 1;
   fmt::print("using cpuid: {}\n", cpuid);
   nostromo::ThreadUtils::SetAffinity(cpuid);

   namespace itch = order_book::itch;

   for (const auto &fname: itch::DATA_FILE_NAMES) {
      fmt::print("processing: {}\n", fname);
      itch::perf_test::PerfTest{fname, out}.Run();
   }

   return 0;
}
