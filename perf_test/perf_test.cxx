#define ALWAYS_INLINE __REPLACE__ALWAYS_INLINE__
#define INLINE __REPLACE__INLINE__

#include "itch/common.hpp"
#include "itch/itch.hpp"
#include "itch/metadata_io.hpp"

#include "itch/v26/order_book.hpp"

#include "nostromo/mmap.hpp"
#include "nostromo/time_utils.hpp"
#include "nostromo/huge_page.hpp"
#include "nostromo/thread_utils.hpp"

#include <fmt/format.h>

#include <cstdint>
#include <stdexcept>
#include <string>

namespace order_book::itch::perf_test {

struct Args {
   std::string fpath = "/tmp/PerfTest-default.csv";
   std::string id = "default";
   std::string version = "v26";
   std::string meta_suffix = "-reverse-bid";
   std::string bin_suffix = "-sorted-idx-reverse-bid";
   int iters = 1;
};

class PerfTest {
   const Args &args_;
   const std::string &fname_;
   std::ofstream &out_;

public:
   explicit PerfTest(
         const Args &args,
         const std::string &fname,
         std::ofstream &out)
         : args_{args},
           fname_{fname},
           out_{out} {}

   template<typename BOOKS>
   void Execute(
         const size_t orders_page_size = nos::HugePage::SIZE_1GB,
         const size_t other_page_size = nos::HugePage::SIZE_2MB) {
      const auto start = nos::TimeUtils::Now();
      const auto fprefix = itch::DATA_DIR_BASE + fname_;

      const auto [
            max_order_id,
            max_stock_code,
            stock_price_map
      ] = MetadataIO::Read(fprefix + ".meta" + args_.meta_suffix);

      BOOKS books{max_order_id, max_stock_code, stock_price_map, orders_page_size, other_page_size};

      const nos::Mmap<char> mmap{fprefix + ".bin" + args_.bin_suffix};
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

      const auto elap = nos::TimeUtils::Now() - start;
      const auto npo = static_cast<double>(elap.count()) / static_cast<double>(order_cnt);

      out_ << args_.id << "," << args_.version << "," << fname_ << "," <<
           elap.count() << "," << order_cnt << "," << npo << std::endl;
      fmt::print("version: {}  ns/order: {:.4f}\n", args_.version, npo);
   }

   static auto ParseArgs(int argc, char **argv) {
      Args args{};

      if (argc == 7) {
         args.fpath = argv[1];
         args.id = argv[2];
         args.version = argv[3];
         args.meta_suffix = argv[4];
         args.bin_suffix = argv[5];
         args.iters = std::stoi(argv[6]);
      }

      return args;
   }
};

} // namespace order_book::itch::perf_test

int main(int argc, char **argv) {
   namespace itch = order_book::itch;

   const auto args = itch::perf_test::PerfTest::ParseArgs(argc, argv);
   std::ofstream out(args.fpath, std::ios_base::app);

   const auto cpuid = static_cast<int>(std::thread::hardware_concurrency()) - 1;
   fmt::print("using cpuid: {}\n", cpuid);
   nos::ThreadUtils::SetAffinity(cpuid);

   for (const auto &fname: itch::DATA_FILE_NAMES) {
      fmt::print("processing: {}\n", fname);
      auto test = itch::perf_test::PerfTest{args, fname, out};

      for (auto x = 0; x < args.iters; ++x) {
         test.Execute<itch::v26::OrderBooks>();
      }
   }
}
