#include "utils.hpp"
#include "itch/itch.hpp"
#include "itch/metadata_io.hpp"

#include "nostromo/mmap.hpp"
#include "nostromo/time_utils.hpp"

#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>

#include <fmt/format.h>

#include <fstream>

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
      fmt::print("processing: {}\n", fpath);
   }

   void Run() {
      const auto start = nostromo::TimeUtils::Now();

      ReadOrders();
      ExportOrdersSorted("-sorted");
      ExportOrdersSortedIdx("-sorted-idx");
      MetadataIO::Write(fpath_, max_order_id_, max_stock_code_, stock_prices_);

      const auto elap = nostromo::TimeUtils::Now() - start;
      const auto ops = Utils::Ops(elap.count(), order_cnt_);

      fmt::print("{}", fmt::format(
            std::locale("en_US.UTF-8"),
            "stock count: {:L}\n"
            "order count: {:L}\n"
            "elapsed: {:L} ns\n"
            "orders/sec: {:L}\n\n",
            stock_orders_.size(), order_cnt_, elap.count(), ops
      ));
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
               const auto order = reinterpret_cast<ItchOrderAdd *>(&data[offset]);
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
               const auto order = reinterpret_cast<ItchOrderExecuted *>(&data[offset]);
               auto &orders = stock_orders_[order->stock_code];
               orders.emplace_back(msg_type, order);

               offset += 18;
               break;
            }
            case 'X': {
               const auto order = reinterpret_cast<ItchOrderCancel *>(&data[offset]);
               auto &orders = stock_orders_[order->stock_code];
               orders.emplace_back(msg_type, order);

               offset += 18;
               break;
            }
            case 'D': {
               const auto order = reinterpret_cast<ItchOrderDelete *>(&data[offset]);
               auto &orders = stock_orders_[order->stock_code];
               orders.emplace_back(msg_type, order);

               offset += 14;
               break;
            }
            case 'U': {
               const auto order = reinterpret_cast<ItchOrderReplace *>(&data[offset]);
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

   void ExportOrdersSorted(const std::string &fext) {
      std::ofstream out(fpath_ + fext, std::ios_base::out | std::ios_base::binary);

      for (const auto &[stock_code, orders]: stock_orders_) {
         for (const auto &[msg_type, order]: orders) {
            out.write((const char *) &msg_type, sizeof(char));

            switch (msg_type) {
               case 'A':
               case 'F':
                  out.write((const char *) order, sizeof(ItchOrderAdd));
                  break;
               case 'E':
               case 'C':
                  out.write((const char *) order, sizeof(ItchOrderExecuted));
                  break;
               case 'X':
                  out.write((const char *) order, sizeof(ItchOrderCancel));
                  break;
               case 'D':
                  out.write((const char *) order, sizeof(ItchOrderDelete));
                  break;
               case 'U':
                  out.write((const char *) order, sizeof(ItchOrderReplace));
                  break;
               default:
                  throw std::runtime_error("unsupported msg_type: " + std::to_string(msg_type));
            }
         }
      }

      if (out.fail()) {
         throw nostromo::Error("write() failed", EX_INFO);
      }
   }

   void ExportOrdersSortedIdx(const std::string &fext) {
      const auto set2map = [](const auto &prices) {
         MAP<uint32_t, uint16_t> map{};
         uint16_t idx = 0u;
         for (const auto price: prices) {
            map[price] = idx;
            ++idx;
         }
         return map;
      };

      std::ofstream out(fpath_ + fext, std::ios_base::out | std::ios_base::binary);

      for (const auto &[stock_code, orders]: stock_orders_) {
         const auto &[bid_set, ask_set] = stock_prices_[stock_code];
         auto bids = set2map(bid_set);
         auto asks = set2map(ask_set);

         for (const auto &[msg_type, order]: orders) {
            out.write((const char *) &msg_type, sizeof(char));

            switch (msg_type) {
               case 'A':
               case 'F': {
                  const auto o = (const ItchOrderAdd *) order;
                  const auto price_idx = o->bid ? bids[o->price] : asks[o->price];
                  const auto enhanced = ItchOrderAddIdx{
                        {{o->timestamp, o->order_id, o->stock_code},
                         o->bid, o->quantity, o->price},
                        price_idx
                  };
                  out.write((const char *) &enhanced, sizeof(ItchOrderAddIdx));
                  break;
               }
               case 'E':
               case 'C':
                  out.write((const char *) order, sizeof(ItchOrderExecuted));
                  break;
               case 'X':
                  out.write((const char *) order, sizeof(ItchOrderCancel));
                  break;
               case 'D':
                  out.write((const char *) order, sizeof(ItchOrderDelete));
                  break;
               case 'U': {
                  const auto o = (const ItchOrderReplace *) order;
                  const auto price_idx = bid_map_[o->order_id] ? bids[o->price] : asks[o->price];
                  const auto enhanced = ItchOrderReplaceIdx{
                        {{o->timestamp, o->order_id, o->stock_code},
                         o->new_order_id, o->quantity, o->price},
                        price_idx
                  };
                  out.write((const char *) &enhanced, sizeof(ItchOrderReplaceIdx));
                  break;
               }
               default:
                  throw std::runtime_error("unsupported msg_type: " + std::to_string(msg_type));
            }
         }
      }

      if (out.fail()) {
         throw nostromo::Error("write() failed", EX_INFO);
      }
   }
};

} // namespace order_book::itch

int main() {
   namespace itch = order_book::itch;

   for (const auto &fname: itch::DATA_FILE_NAMES) {
      const auto fpath = itch::DATA_DIR_BASE + fname + ".bin";
      order_book::itch::ItchPreProcessor{fpath}.Run();
   }

   return 0;
}