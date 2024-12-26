#include <iostream>
#include <map>
#include <numeric>

#include "nostromo/mmap.hpp"

#include "utils.hpp"
#include "itch/metadata_io.hpp"

namespace order_book::itch {

class PriceAnalyzer {

public:
   static void Run(std::string &fpath) {
      std::cout << "processing: " << fpath << std::endl;
      auto tuple = MetadataIO::Read(fpath);
      auto &stock_prices = std::get<2>(tuple);

      for (auto &[stock_code, pair]: stock_prices) {
         std::cout << "stock_code: " << stock_code << std::endl;
         auto &[bids, asks] = pair;
         auto avg = std::accumulate(bids.begin(), bids.end(), 0.0) /
                    static_cast<double>(bids.size());
         std::cout << avg << std::endl;
         std::cout << avg << std::endl;

         for (auto bid: bids) {
            std::cout << bid << std::endl;
         }
         sleep(1);
      }
   }
};

} // namespace order_book::itch

int main() {
   std::cout.imbue(std::locale(""));
   std::string base_dir = "/remote/data/nasdaq-itch/";

   auto fnames = {
//         "01302019.NASDAQ_ITCH50.meta",
//         "01302020.NASDAQ_ITCH50.meta",
         "12302019.NASDAQ_ITCH50.meta"
   };

   for (auto &fname: fnames) {
      auto fpath = base_dir + fname;
      order_book::itch::PriceAnalyzer::Run(fpath);
   }
}