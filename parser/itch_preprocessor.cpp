#include "utils.hpp"
#include "itch/common.hpp"
#include "itch/using.hpp"
#include "itch/itch.hpp"
#include "itch/metadata_io.hpp"

#include "nostromo/mmap.hpp"
#include "nostromo/time_utils.hpp"

#include <fmt/format.h>

#include <fstream>
#include <set>

namespace order_book::itch {

using STOCK_ORDER_UMAP = UMAP<uint16_t, std::vector<std::pair<char, ItchBase *>>>;

class ItchPreprocessor {
   const std::string &fprefix_;
   const nostromo::Mmap<char> mmap_;

   STOCK_ORDER_UMAP stock_order_umap_;
   UMAP<uint32_t, uint8_t> bid_umap_;

   uint32_t max_order_id_ = 0;
   uint16_t max_stock_code_ = 0;
   uint64_t order_cnt_ = 0;

public:
   explicit ItchPreprocessor(const std::string &fprefix)
         : fprefix_{fprefix},
           mmap_{fprefix + ".bin"} {}

   void Run() {
      const auto start = nostromo::TimeUtils::Now();

      STOCK_PRICE_UMAP stock_price_umap = ReadOrders();

      STOCK_PRICE_MAP stock_price_map{};
      STOCK_PRICE_MAP stock_price_map_reverse_bid{};
      STOCK_PRICE_MAP stock_price_map_reverse_ask{};
      STOCK_PRICE_MAP stock_price_map_busiest{};

      uint16_t busiest_stock_code = 0;
      size_t busiest_stock_cnt = 0;

      for (const auto &[stock_code, pair]: stock_price_umap) {
         const auto &[ask_uset, bid_uset] = pair;

         const auto ask_set = std::set<uint32_t>{ask_uset.begin(), ask_uset.end()};
         const auto bid_set = std::set<uint32_t>{bid_uset.begin(), bid_uset.end()};

         const auto asks = std::vector<uint32_t>{ask_set.begin(), ask_set.end()};
         const auto bids = std::vector<uint32_t>{bid_set.begin(), bid_set.end()};

         const auto asks_reversed = std::vector<uint32_t>{asks.rbegin(), asks.rend()};
         const auto bids_reversed = std::vector<uint32_t>{bids.rbegin(), bids.rend()};

         assert(ask_uset.size() == ask_set.size());
         assert(ask_uset.size() == asks.size());
         assert(ask_uset.size() == asks_reversed.size());
         assert(bid_uset.size() == bid_set.size());
         assert(bid_uset.size() == bids.size());
         assert(bid_uset.size() == bids_reversed.size());

         stock_price_map[stock_code] = std::make_pair(asks, bids);
         stock_price_map_reverse_ask[stock_code] = std::make_pair(asks_reversed, bids);
         stock_price_map_reverse_bid[stock_code] = std::make_pair(asks, bids_reversed);

         const auto cnt = asks.size() + bids.size();
         assert(cnt > 0);
         if (cnt > busiest_stock_cnt) {
            busiest_stock_cnt = cnt;
            busiest_stock_code = stock_code;
         }
      }

      stock_price_map_busiest[busiest_stock_code] = stock_price_map[busiest_stock_code];

      const auto fpath_bin = fprefix_ + ".bin-sorted";
      const auto fpath_meta = fprefix_ + ".meta";

      ExportOrdersSorted(fpath_bin);
      ExportOrdersSortedIdx(fpath_bin + "-idx", stock_price_map);
      ExportOrdersSortedIdx(fpath_bin + "-idx-reverse-ask", stock_price_map_reverse_ask);
      ExportOrdersSortedIdx(fpath_bin + "-idx-reverse-bid", stock_price_map_reverse_bid);

      MetadataIO::Write(fpath_meta, max_order_id_, max_stock_code_, stock_price_map);
      MetadataIO::Write(fpath_meta + "-reverse-ask", max_order_id_, max_stock_code_, stock_price_map_reverse_ask);
      MetadataIO::Write(fpath_meta + "-reverse-bid", max_order_id_, max_stock_code_, stock_price_map_reverse_bid);

      const auto elap = nostromo::TimeUtils::Now() - start;
      const auto ops = Utils::Ops(elap.count(), order_cnt_);

      fmt::print("{}", fmt::format(
            std::locale("en_US.UTF-8"),
            "stock count: {:L}\n"
            "order count: {:L}\n"
            "elapsed: {:L} ns\n"
            "orders/sec: {:L}\n\n",
            stock_order_umap_.size(), order_cnt_, elap.count(), ops
      ));
   }

private:
   STOCK_PRICE_UMAP ReadOrders() {
      const auto data = mmap_.Span();
      STOCK_PRICE_UMAP stock_price_umap;
      uint64_t offset = 0;

      while (offset < data.size()) {
         const auto msg_type = data[offset];
         ++offset;
         ++order_cnt_;

         switch (msg_type) {
            case 'A':
            case 'F': {
               const auto order = reinterpret_cast<ItchOrderAdd *>(&data[offset]);

               auto stock_exists = stock_order_umap_.contains(order->stock_code);
               auto &orders = stock_order_umap_[order->stock_code];
               if (stock_exists) assert(!orders.empty());

               orders.emplace_back(msg_type, order);

               if (order->order_id > max_order_id_) max_order_id_ = order->order_id;
               if (order->stock_code > max_stock_code_) max_stock_code_ = order->stock_code;

               auto &[ask_uset, bid_uset] = stock_price_umap[order->stock_code];
               if (stock_exists) assert(!ask_uset.empty() || !bid_uset.empty());

               auto &price_uset = order->bid ? bid_uset : ask_uset;
               price_uset.insert(order->price);

               assert(!bid_umap_.contains(order->order_id));
               bid_umap_[order->order_id] = order->bid;

               offset += sizeof(ItchOrderAdd);
               break;
            }
            case 'E':
            case 'C': {
               const auto order = reinterpret_cast<ItchOrderExecuted *>(&data[offset]);
               auto &orders = stock_order_umap_.at(order->stock_code);
               orders.emplace_back(msg_type, order);
               offset += sizeof(ItchOrderExecuted);
               break;
            }
            case 'X': {
               const auto order = reinterpret_cast<ItchOrderCancel *>(&data[offset]);
               auto &orders = stock_order_umap_.at(order->stock_code);
               orders.emplace_back(msg_type, order);
               offset += sizeof(ItchOrderCancel);
               break;
            }
            case 'D': {
               const auto order = reinterpret_cast<ItchOrderDelete *>(&data[offset]);
               auto &orders = stock_order_umap_.at(order->stock_code);
               orders.emplace_back(msg_type, order);
               offset += sizeof(ItchOrderDelete);
               break;
            }
            case 'U': {
               const auto order = reinterpret_cast<ItchOrderReplace *>(&data[offset]);
               auto &orders = stock_order_umap_.at(order->stock_code);
               orders.emplace_back(msg_type, order);

               if (order->order_id > max_order_id_) max_order_id_ = order->order_id;
               if (order->stock_code > max_stock_code_) max_stock_code_ = order->stock_code;

               const auto bid = bid_umap_.at(order->order_id);
               auto &[ask_uset, bid_uset] = stock_price_umap.at(order->stock_code);
               assert(!ask_uset.empty() || !bid_uset.empty());

               auto &price_uset = bid ? bid_uset : ask_uset;

               price_uset.insert(order->price);
               assert(!bid_umap_.contains(order->new_order_id));
               bid_umap_[order->new_order_id] = bid;

               offset += sizeof(ItchOrderReplace);
               break;
            }
            default:
               throw std::runtime_error("unexpected msg_type at offset: " + std::to_string(offset - 1));
         }
      }

      return stock_price_umap;
   }

   void ExportOrdersSorted(const std::string &fpath) {
      std::ofstream out(fpath, std::ios_base::out | std::ios_base::binary);

      for (const auto &[stock_code, orders]: stock_order_umap_) {
         assert(!orders.empty());

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

            if (out.fail()) {
               throw nostromo::Error("write() failed", EX_INFO);
            }
         }
      }
   }

   auto Set2map(const auto &prices) {
      UMAP<uint32_t, uint16_t> map{};
      uint16_t idx = 0u;
      for (const auto price: prices) {
         map[price] = idx;
         ++idx;
      }
      return map;
   };

   void ExportOrdersSortedIdx(const std::string &fpath, STOCK_PRICE_MAP &stock_price_map) {
      std::ofstream out(fpath, std::ios_base::out | std::ios_base::binary);

      for (const auto &[stock_code, orders]: stock_order_umap_) {
         const auto &[asks, bids] = stock_price_map[stock_code];
         assert(!asks.empty() || !bids.empty());

         auto ask_map = Set2map(asks);
         auto bid_map = Set2map(bids);
         assert(asks.size() == ask_map.size() && bids.size() == bid_map.size());

         for (const auto &[msg_type, order]: orders) {
            out.write((const char *) &msg_type, sizeof(char));

            switch (msg_type) {
               case 'A':
               case 'F': {
                  const auto o = (const ItchOrderAdd *) order;
                  const auto price_idx = o->bid ? bid_map.at(o->price) : ask_map.at(o->price);
                  const auto idx_order = ItchOrderAddIdx{
                        {{o->timestamp, o->order_id, o->stock_code},
                         o->bid, o->quantity, o->price},
                        price_idx};
                  out.write((const char *) &idx_order, sizeof(ItchOrderAddIdx));
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
                  const auto price_idx = bid_umap_.at(o->order_id) ? bid_map.at(o->price) : ask_map.at(o->price);
                  const auto idx_order = ItchOrderReplaceIdx{
                        {{o->timestamp, o->order_id, o->stock_code},
                         o->new_order_id, o->quantity, o->price},
                        price_idx};
                  out.write((const char *) &idx_order, sizeof(ItchOrderReplaceIdx));
                  break;
               }
               default:
                  throw std::runtime_error("unsupported msg_type: " + std::to_string(msg_type));
            }

            if (out.fail()) {
               throw nostromo::Error("write() failed", EX_INFO);
            }
         }
      }
   }
};

} // namespace order_book::itch

int main() {
   namespace itch = order_book::itch;

   for (const auto &fname: itch::DATA_FILE_NAMES) {
      const auto fprefix = itch::DATA_DIR_BASE + fname;
      fmt::print("processing: {}\n", fprefix);
      itch::ItchPreprocessor{fprefix}.Run();
   }

   return 0;
}