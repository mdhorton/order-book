#include <fstream>
#include <iostream>
#include <map>

#include "nostromo/mmap.hpp"
#include "nostromo/time_utils.hpp"

#include "itch/itch.hpp"
#include "itch/v02/order_book.hpp"

#define SET boost::unordered_flat_set

namespace order_book::itch {

class MetadataCreator {
private:
   static auto CreateMetaData(std::string &fpath) {
      auto start = nostromo::TimeUtils::Now();

      nostromo::Mmap<char> mmap{fpath};
      auto data = mmap.Ptr();
      auto fsize = mmap.Size();

      MAP<uint32_t, uint8_t> bid_map;
      // stock_code -> pair<bid_prices, ask_prices>
      MAP<uint16_t, std::pair<SET<uint32_t>, SET<uint32_t>>> stock_prices;
      uint32_t max_order_id = 0;
      uint64_t offset = 0;
      uint64_t order_cnt = 0;

      while (offset < fsize) {
         ++order_cnt;
         auto msg_type = data[offset++];

         switch (msg_type) {
            case 'A':
            case 'F': {
               auto order = (ItchOrderAdd *) &data[offset];
               if (order->order_id > max_order_id) {
                  max_order_id = order->order_id;
               }

               auto &pair = stock_prices[order->stock_code];
               auto &prices = order->bid ? pair.first : pair.second;

               prices.insert(order->price);
               bid_map[order->order_id] = order->bid;
               offset += 23;
               break;
            }
            case 'E':
            case 'C':
            case 'X': {
               offset += 18;
               break;
            }
            case 'D': {
               offset += 14;
               break;
            }
            case 'U': {
               auto order = (ItchOrderReplace *) &data[offset];
               if (order->new_order_id > max_order_id) {
                  max_order_id = order->new_order_id;
               }

               auto bid = bid_map[order->order_id];
               auto &pair = stock_prices[order->stock_code];
               auto &prices = bid ? pair.first : pair.second;

               prices.insert(order->price);
               bid_map[order->new_order_id] = bid;
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
            << "CreateMetaData" << std::endl
            << "stock count: " << stock_prices.size() << std::endl
            << "order count: " << order_cnt << std::endl
            << "elapsed: " << elap.count() << std::endl
            << "orders/sec: " << ops << std::endl
            << std::endl;

      return std::make_pair(max_order_id, stock_prices);
   }

   static auto SortMetaData(auto &stock_prices) {
      auto start = nostromo::TimeUtils::Now();

      std::map<uint16_t, std::pair<std::set<uint32_t>, std::set<uint32_t>>> sorted;

      for (auto &[stock_code, pair]: stock_prices) {
         auto &bids = pair.first;
         auto &asks = pair.second;
         std::set<uint32_t> bid_prices{bids.begin(), bids.end()};
         std::set<uint32_t> ask_prices{asks.begin(), asks.end()};
         sorted[stock_code] = std::make_pair(bid_prices, ask_prices);
      }

      auto stop = nostromo::TimeUtils::Now();
      auto elap = stop - start;

      std::cout
            << "SortMetaData" << std::endl
            << "elapsed: " << elap.count() << std::endl
            << std::endl;

      return sorted;
   }

   static void ExportMetaData(
         std::string &fpath,
         uint32_t max_order_id,
         auto &stock_prices) {
      auto start = nostromo::TimeUtils::Now();

      auto in_path = RemoveExtension(fpath) + ".meta";
      std::ofstream out_bin(in_path, std::ios_base::out | std::ios_base::binary);

      out_bin.write((char *) &max_order_id, sizeof(max_order_id));

      auto stock_cnt = stock_prices.size();
      out_bin.write((char *) &stock_cnt, sizeof(stock_cnt));

      for (auto &[stock_code, pair]: stock_prices) {
         out_bin.write((const char *) &stock_code, sizeof(stock_code));
         WritePrices(out_bin, pair.first);
         WritePrices(out_bin, pair.second);
      }

      if (out_bin.fail()) {
         throw nostromo::Error("write() failed", EX_INFO);
      }

      auto stop = nostromo::TimeUtils::Now();
      auto elap = stop - start;

      std::cout
            << "ExportMetaData" << std::endl
            << "elapsed: " << elap.count() << std::endl
            << std::endl;
   }

   static void WritePrices(std::ofstream &out, std::set<uint32_t> &prices) {
      auto price_cnt = prices.size();
      out.write((char *) &price_cnt, sizeof(price_cnt));
      for (auto price: prices) {
         out.write((char *) &price, sizeof(price));
      }
   }

   static std::string RemoveExtension(std::string &path) {
      auto pos = path.find_last_of('.');
      if (pos <= 0) return path;
      return path.substr(0, pos);
   }

public:
   static void Run(std::string &fpath) {
      std::cout << "processing: " << fpath << std::endl;
      auto [max_order_id, stock_prices] = CreateMetaData(fpath);
      auto sorted = SortMetaData(stock_prices);
      ExportMetaData(fpath, max_order_id, sorted);
   }
};

} // namespace order_book::itch

int main() {
   using order_book::itch::MetadataCreator;

   std::cout.imbue(std::locale(""));
   std::string base_dir = "/remote/data/nasdaq-itch/";

   auto fnames = {
         "01302019.NASDAQ_ITCH50.bin",
         "01302020.NASDAQ_ITCH50.bin",
         "12302019.NASDAQ_ITCH50.bin"
   };

   for (auto &fname: fnames) {
      auto fpath = base_dir + fname;
      MetadataCreator::Run(fpath);
   }

   return 0;
}