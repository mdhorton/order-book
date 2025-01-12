#include "utils.hpp"
#include "itch/itch.hpp"
#include "itch/metadata_io.hpp"

#include "nostromo/mmap.hpp"
#include "nostromo/time_utils.hpp"

#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>

#include <fstream>
#include <iostream>

namespace order_book::itch {

template<typename K, typename V>
using MAP = boost::unordered_flat_map<K, V>;

template<typename K>
using SET = boost::unordered_flat_set<K>;

using STOCK_ORDERS = MAP<uint16_t, std::vector<std::pair<char, ItchBase *>>>;
using STOCK_PRICES = MAP<uint16_t, std::pair<SET<uint32_t>, SET<uint32_t>>>;

class ItchPreProcessor {
public:
   explicit ItchPreProcessor(const std::string &fpath)
         : fpath_{fpath},
           mmap_{fpath} {
      std::cout << "processing: " << fpath << std::endl;
   }

   void Run() {
      const auto start = nostromo::TimeUtils::Now();

      ReadOrders();
      ExportOrders();
      MetadataIO::Write(fpath_, max_order_id_, max_stock_code_, stock_prices_);

      const auto stop = nostromo::TimeUtils::Now();
      const auto elap = stop - start;
      const auto elap_d = static_cast<double>(elap.count());
      const auto order_cnt_d = static_cast<double>(order_cnt_);
      const auto ops = static_cast<uint64_t>(order_cnt_d / (elap_d / 1'000'000'000.0));

      std::cout
            << "stock count: " << stock_orders_.size() << std::endl
            << "order count: " << order_cnt_ << std::endl
            << "elapsed: " << elap.count() << std::endl
            << "orders/sec: " << ops << std::endl
            << std::endl;
   }

private:
   const std::string &fpath_;
   const nostromo::Mmap<char> mmap_;

   STOCK_ORDERS stock_orders_;
   STOCK_PRICES stock_prices_;
   MAP<uint32_t, uint8_t> bid_map_;

   uint32_t max_order_id_ = 0;
   uint16_t max_stock_code_ = 0;
   uint64_t order_cnt_ = 0;

   void ReadOrders() {
      const auto data = mmap_.Span();
      uint64_t offset = 0;

      while (offset < data.size()) {
         const auto msg_type = data[offset];
         ++offset;
         ++order_cnt_;

         switch (msg_type) {
            case 'A':
            case 'F': {
               const auto order = (ItchOrderAdd *) &data[offset];
               auto &orders = stock_orders_[order->stock_code];
               orders.emplace_back(msg_type, order);

               if (order->order_id > max_order_id_) max_order_id_ = order->order_id;
               if (order->stock_code > max_stock_code_) max_stock_code_ = order->stock_code;

               auto &pair = stock_prices_[order->stock_code];
               auto &prices = order->bid ? pair.first : pair.second;

               prices.insert(order->price);
               bid_map_[order->order_id] = order->bid;

               offset += 23;
               break;
            }
            case 'E':
            case 'C': {
               const auto order = (ItchOrderExecuted *) &data[offset];
               auto &orders = stock_orders_[order->stock_code];
               orders.emplace_back(msg_type, order);

               offset += 18;
               break;
            }
            case 'X': {
               const auto order = (ItchOrderCancel *) &data[offset];
               auto &orders = stock_orders_[order->stock_code];
               orders.emplace_back(msg_type, order);

               offset += 18;
               break;
            }
            case 'D': {
               const auto order = (ItchOrderDelete *) &data[offset];
               auto &orders = stock_orders_[order->stock_code];
               orders.emplace_back(msg_type, order);

               offset += 14;
               break;
            }
            case 'U': {
               const auto order = (ItchOrderReplace *) &data[offset];
               auto &orders = stock_orders_[order->stock_code];
               orders.emplace_back(msg_type, order);

               if (order->order_id > max_order_id_) max_order_id_ = order->order_id;
               if (order->stock_code > max_stock_code_) max_stock_code_ = order->stock_code;

               const auto bid = bid_map_[order->order_id];
               auto &pair = stock_prices_[order->stock_code];
               auto &prices = bid ? pair.first : pair.second;

               prices.insert(order->price);
               bid_map_[order->new_order_id] = bid;

               offset += 26;
               break;
            }
            default:
               throw std::runtime_error("unexpected msg_type at offset: " + std::to_string(offset - 1));
         }
      }
   }

   auto PriceMap(const auto &prices) {
      MAP<uint32_t, uint16_t> map{};
      uint16_t idx = 0u;
      for (const auto price: prices) {
         map[price] = idx;
         ++idx;
      }
      return map;
   }

   void ExportOrders() {
      const auto out_path = Utils::RemoveExtension(fpath_) + ".preprocessed";
      std::ofstream out_bin(out_path, std::ios_base::out | std::ios_base::binary);

      for (const auto &[stock_code, orders]: stock_orders_) {
         const auto &[bid_set, ask_set] = stock_prices_[stock_code];
         auto bids = PriceMap(bid_set);
         auto asks = PriceMap(ask_set);

         for (const auto &[msg_type, order]: orders) {
            out_bin.write((const char *) &msg_type, sizeof(msg_type)); // TODO: sizeof()

            switch (msg_type) {
               case 'A':
               case 'F': {
                  const auto o = (const ItchOrderAdd *) order;
                  const auto price_idx = o->bid ? bids[o->price] : asks[o->price];
                  const auto enhanced = ItchOrderAddEnhanced{
                        {{o->stock_code, o->timestamp, o->order_id},
                         o->bid, o->quantity, o->price},
                        price_idx
                  };
                  out_bin.write((const char *) &enhanced, sizeof(ItchOrderAddEnhanced));
                  break;
               }
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
               case 'U': {
                  const auto o = (const ItchOrderReplace *) order;
                  const auto price_idx = bid_map_[o->order_id] ? bids[o->price] : asks[o->price];
                  const auto enhanced = ItchOrderReplaceEnhanced{
                        {{o->stock_code, o->timestamp, o->order_id},
                         o->new_order_id, o->quantity, o->price},
                        price_idx
                  };
                  out_bin.write((const char *) &enhanced, sizeof(ItchOrderReplaceEnhanced));
                  break;
               }
               default:
                  throw std::runtime_error("unexpected msg_type: " + std::to_string(msg_type));
            }
         }
      }

      if (out_bin.fail()) {
         throw nostromo::Error("write() failed", EX_INFO);
      }
   }
};

} // namespace order_book::itch

int main() {
   std::cout.imbue(std::locale(""));
   const std::string base_dir = "/remote/data/nasdaq-itch/";

   const auto fnames = {
//         "01302019.NASDAQ_ITCH50.bin",
//         "01302020.NASDAQ_ITCH50.bin",
         "12302019.NASDAQ_ITCH50.bin"
   };

   for (const auto &fname: fnames) {
      order_book::itch::ItchPreProcessor{base_dir + fname}.Run();
   }

   return 0;
}