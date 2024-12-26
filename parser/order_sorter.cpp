#include <fstream>
#include <iostream>

#include <boost/unordered/unordered_flat_map.hpp>

#include "nostromo/mmap.hpp"
#include "nostromo/time_utils.hpp"

#include "utils.hpp"
#include "itch/itch.hpp"

namespace order_book::itch {

using STOCK_ORDERS = boost::unordered_flat_map<uint16_t, std::vector<std::pair<char, ItchBase *>>>;

class OrderSorter {
private:
   static void SortOrders(std::string &fpath) {
      auto start = nostromo::TimeUtils::Now();

      nostromo::Mmap<char> mmap{fpath};
      auto data = mmap.Span();

      STOCK_ORDERS stock_orders;
      uint64_t offset = 0;
      uint64_t order_cnt = 0;

      while (offset < data.size()) {
         ++order_cnt;
         auto msg_type = data[offset++];

         switch (msg_type) {
            case 'A':
            case 'F': {
               auto order = (ItchOrderAdd *) &data[offset];
               (&stock_orders[order->stock_code])->emplace_back(msg_type, order);
               offset += 23;
               break;
            }
            case 'E':
            case 'C': {
               auto order = (ItchOrderExecuted *) &data[offset];
               (&stock_orders[order->stock_code])->emplace_back(msg_type, order);
               offset += 18;
               break;
            }
            case 'X': {
               auto order = (ItchOrderCancel *) &data[offset];
               (&stock_orders[order->stock_code])->emplace_back(msg_type, order);
               offset += 18;
               break;
            }
            case 'D': {
               auto order = (ItchOrderDelete *) &data[offset];
               (&stock_orders[order->stock_code])->emplace_back(msg_type, order);
               offset += 14;
               break;
            }
            case 'U': {
               auto order = (ItchOrderReplace *) &data[offset];
               (&stock_orders[order->stock_code])->emplace_back(msg_type, order);
               offset += 26;
               break;
            }
            default:
               throw std::runtime_error("unexpected msg_type at offset: " + std::to_string(offset - 1));
         }
      }

      ExportMetaData(fpath, stock_orders);

      auto stop = nostromo::TimeUtils::Now();
      auto elap = stop - start;
      auto ops = static_cast<uint64_t>(
            static_cast<double>(order_cnt) /
            (static_cast<double>(elap.count()) / 1'000'000'000));

      std::cout
            << "CreateMetaData" << std::endl
            << "stock count: " << stock_orders.size() << std::endl
            << "order count: " << order_cnt << std::endl
            << "elapsed: " << elap.count() << std::endl
            << "orders/sec: " << ops << std::endl
            << std::endl;
   }

   static void ExportMetaData(std::string &fpath, STOCK_ORDERS &stock_orders) {
      auto out_path = Utils::RemoveExtension(fpath) + ".sorted-bin";
      std::ofstream out_bin(out_path, std::ios_base::out | std::ios_base::binary);

      for (auto &[stock_code, orders]: stock_orders) {
         for (auto [msg_type, order]: orders) {
            out_bin.write((const char *) &msg_type, sizeof(msg_type));

            switch (msg_type) {
               case 'A':
               case 'F':
                  out_bin.write((const char *) order, sizeof(ItchOrderAdd));
                  break;
               case 'E':
               case 'C':
                  out_bin.write((const char *) order, sizeof(ItchOrderExecuted));
                  break;
               case 'X':
                  out_bin.write((const char *) order, sizeof(ItchOrderCancel));
                  break;
               case 'D':
                  out_bin.write((const char *) order, sizeof(ItchOrderDelete));
                  break;
               case 'U':
                  out_bin.write((const char *) order, sizeof(ItchOrderReplace));
                  break;
               default:
                  throw std::runtime_error("unexpected msg_type: " + std::to_string(msg_type));
            }
         }
      }

      if (out_bin.fail()) {
         throw nostromo::Error("write() failed", EX_INFO);
      }
   }

public:
   static void Run(std::string &fpath) {
      std::cout << "processing: " << fpath << std::endl;
      SortOrders(fpath);
   }
};

} // namespace order_book::itch

int main() {
   std::cout.imbue(std::locale(""));
   std::string base_dir = "/remote/data/nasdaq-itch/";

   auto fnames = {
         "01302019.NASDAQ_ITCH50.bin",
         "01302020.NASDAQ_ITCH50.bin",
         "12302019.NASDAQ_ITCH50.bin"
   };

   for (auto &fname: fnames) {
      auto fpath = base_dir + fname;
      order_book::itch::OrderSorter::Run(fpath);
   }

   return 0;
}