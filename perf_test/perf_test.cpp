#include <cstdint>
#include <stdexcept>
#include <string>
#include <iostream>
#include <fstream>
#include <set>

#include "nostromo/mmap.hpp"
#include "nostromo/time_utils.hpp"

#include "itch/itch.hpp"
#include "itch/v02/order_book.hpp"

namespace order_book::itch::v02::perf_test {

class PerfTest {
private:
   template<typename T>
   static auto ReadInt(std::ifstream &in) {
      T obj{};
      auto n = (std::streamsize) sizeof(T);
      in.read((char *) &obj, n);
      if (in.gcount() != n) {
         throw nostromo::Error("read() failed", EX_INFO);
      }
      return obj;
   }

   static auto ReadPrices(std::ifstream &in) {
      std::set<uint32_t> prices;
      auto price_cnt = ReadInt<size_t>(in);
      for (auto idx = 0u; idx < price_cnt; ++idx) {
         prices.emplace(ReadInt<uint32_t>(in));
      }
      return prices;
   }

   static auto ImportMetaData(std::string &fpath) {
      auto in_path = RemoveExtension(fpath) + ".meta";
      std::ifstream in(in_path, std::ios_base::in | std::ios_base::binary);

      auto max_order_id = ReadInt<uint32_t>(in);
      auto stock_cnt = ReadInt<size_t>(in);
      std::map<uint16_t, std::pair<std::set<uint32_t>, std::set<uint32_t>>> stock_prices;

      for (auto idx = 0u; idx < stock_cnt; ++idx) {
         auto stock_code = ReadInt<uint16_t>(in);
         auto bids = ReadPrices(in);
         auto asks = ReadPrices(in);
         stock_prices[stock_code] = std::make_pair(bids, asks);
      }

      std::cout
            << "stock max_order_id: " << max_order_id << std::endl
            << "stock_cnt: " << stock_cnt << std::endl
            << "stock_price_cnt: " << stock_prices.size() << std::endl;

      return std::make_pair(max_order_id, stock_prices);
   }

   static auto RunPerfTest(std::string &fpath) {
      auto start = nostromo::TimeUtils::Now();

      auto [max_order_id, stock_prices] = ImportMetaData(fpath);

      nostromo::Mmap<char> mmap{fpath, nostromo::HugePageUtils::SIZE_2MB};
      auto data = mmap.Ptr();
      auto fsize = mmap.Size();

      OrderBooks books{max_order_id, stock_prices};

      uint64_t offset = 0;
      uint64_t order_cnt = 0;

      while (offset < fsize) {
         ++order_cnt;

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

   static std::string RemoveExtension(std::string &path) {
      auto pos = path.find_last_of('.');
      if (pos <= 0) return path;
      return path.substr(0, pos);
   }

public:
   static void Run(std::string &fpath) {
      std::cout << "processing: " << fpath << std::endl;
      RunPerfTest(fpath);
   }
};

} // namespace order_book::itch::v02::perf_test

int main() {
   using order_book::itch::v02::perf_test::PerfTest;

   std::cout.imbue(std::locale(""));
   std::string base_dir = "/remote/data/nasdaq-itch/";

   auto fnames = {
//         "01302019.NASDAQ_ITCH50.bin",
//         "01302020.NASDAQ_ITCH50.bin",
         "12302019.NASDAQ_ITCH50.bin"
   };

   for (auto &fname: fnames) {
      auto fpath = base_dir + fname;
      PerfTest::Run(fpath);
   }

   return 0;
}
