#include <fstream>
#include <iostream>

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include "nostromo/mmap.hpp"
#include "nostromo/time_utils.hpp"

#include "itch/itch.hpp"
#include "itch/itch_raw.hpp"
#include "itch/v02/order_book.hpp"

namespace order_book::itch {

class ItchRawParser {
private:
   static void ParseRawFile(std::string &in_path) {
      namespace io = boost::iostreams;
      auto start = nostromo::TimeUtils::Now();

      auto base_fpath = RemoveExtension(in_path);
      std::ofstream out_bin(base_fpath + ".bin", std::ios_base::out | std::ios_base::binary);

      std::ifstream in(in_path, std::ios_base::in | std::ios_base::binary);
      io::filtering_istream filter;
      filter.push(io::gzip_decompressor()); // add decompressor to the filter stack.
      filter.push(in); // add file stream to the filter stack.

      char msg_type;
      char msg[64]; // all messages are less than 64 bytes.
      uint64_t order_cnt = 0;

      while (true) {
         filter.read(&msg_type, 1);
         if (filter.gcount() == 0) {
            if (filter.eof()) break;
            throw nostromo::Error("read failed", EX_INFO);
         }

         switch (msg_type) {
            default:
               break;
            case 'S':
               Read(filter, msg, 11);
               break;
            case 'R':
               Read(filter, msg, 38);
               break;
            case 'H':
               Read(filter, msg, 24);
               break;
            case 'Y':
               Read(filter, msg, 19);
               break;
            case 'L':
               Read(filter, msg, 25);
               break;
            case 'V':
               Read(filter, msg, 34);
               break;
            case 'W':
               Read(filter, msg, 11);
               break;
            case 'K':
               Read(filter, msg, 27);
               break;
            case 'J':
               Read(filter, msg, 34);
               break;
            case 'h':
               Read(filter, msg, 20);
               break;
            case 'A': {
               Read(filter, msg, 35);
               Handle_A(msg, out_bin);
               ++order_cnt;
               break;
            }
            case 'F': {
               Read(filter, msg, 39);
               Handle_F(msg, out_bin);
               ++order_cnt;
               break;
            }
            case 'E': {
               Read(filter, msg, 30);
               Handle_E(msg, out_bin);
               ++order_cnt;
               break;
            }
            case 'C': {
               Read(filter, msg, 35);
               Handle_C(msg, out_bin);
               ++order_cnt;
               break;
            }
            case 'X': {
               Read(filter, msg, 22);
               Handle_X(msg, out_bin);
               ++order_cnt;
               break;
            }
            case 'D': {
               Read(filter, msg, 18);
               Handle_D(msg, out_bin);
               ++order_cnt;
               break;
            }
            case 'U': {
               Read(filter, msg, 34);
               Handle_U(msg, out_bin);
               ++order_cnt;
               break;
            }
            case 'P':
               Read(filter, msg, 43);
               break;
            case 'Q':
               Read(filter, msg, 39);
               break;
            case 'B':
               Read(filter, msg, 18);
               break;
            case 'I':
               Read(filter, msg, 49);
               break;
            case 'N':
               Read(filter, msg, 19);
               break;
            case 'O':
               Read(filter, msg, 47);
               break;
         }
      }

      auto stop = nostromo::TimeUtils::Now();
      auto elap = stop - start;
      auto ops = order_cnt / (elap.count() / 1'000'000'000);

      std::cout
            << "ParseRawFile" << std::endl
            << "order count: " << order_cnt << std::endl
            << "elapsed: " << elap.count() << std::endl
            << "orders/sec: " << ops << std::endl
            << std::endl;
   }

   static void Handle_A(char *msg, std::ofstream &bin_out) {
      auto itch = (ItchRawOrderAdd *) msg;
      ItchOrderAdd order{};
      SetBaseFields(order, *itch);
      order.bid = itch->bid == 'B' ? 1 : 0;
      order.quantity = Quantity(itch->quantity);
      order.price = Price(itch->price);
      Write(bin_out, 'A', &order, sizeof(ItchOrderAdd));
   }

   static void Handle_F(char *msg, std::ofstream &out) {
      auto itch = (ItchRawOrderAddMpid *) msg;
      ItchOrderAdd order{};
      SetBaseFields(order, *itch);
      order.bid = itch->bid == 'B' ? 1 : 0;
      order.quantity = Quantity(itch->quantity);
      order.price = Price(itch->price);
      Write(out, 'F', &order, sizeof(ItchOrderAdd));
   }

   static void Handle_E(char *msg, std::ofstream &out) {
      auto itch = (ItchRawOrderExecuted *) msg;
      ItchOrderExecuted order{};
      SetBaseFields(order, *itch);
      order.quantity = Quantity(itch->quantity);
      Write(out, 'E', &order, sizeof(ItchOrderExecuted));
   }

   static void Handle_C(char *msg, std::ofstream &out) {
      auto itch = (ItchRawOrderExecutedPrice *) msg;
      ItchOrderExecuted order{};
      SetBaseFields(order, *itch);
      order.quantity = Quantity(itch->quantity);
      Write(out, 'C', &order, sizeof(ItchOrderExecuted));
   }

   static void Handle_X(char *msg, std::ofstream &out) {
      auto itch = (ItchRawOrderCancel *) msg;
      ItchOrderCancel order{};
      SetBaseFields(order, *itch);
      order.quantity = Quantity(itch->quantity);
      Write(out, 'X', &order, sizeof(ItchOrderCancel));
   }

   static void Handle_D(char *msg, std::ofstream &out) {
      auto itch = (ItchRawOrderDelete *) msg;
      ItchOrderDelete order{};
      SetBaseFields(order, *itch);
      Write(out, 'D', &order, sizeof(ItchOrderDelete));
   }

   static void Handle_U(char *msg, std::ofstream &out) {
      auto itch = (ItchRawOrderReplace *) msg;
      ItchOrderReplace order{};
      SetBaseFields(order, *itch);
      order.new_order_id = OrderId(itch->new_order_id);
      order.quantity = Quantity(itch->quantity);
      order.price = Price(itch->price);
      Write(out, 'U', &order, sizeof(ItchOrderReplace));
   }

   static auto CreateMetaData(std::string &fpath) {
      auto start = nostromo::TimeUtils::Now();

      auto in_path = RemoveExtension(fpath) + ".bin";

      nostromo::Mmap<char> mmap{in_path};
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
               if (order->order_id > max_order_id) max_order_id = order->order_id;
               auto pair = &stock_prices[order->stock_code];
               auto prices = order->bid ? &pair->first : &pair->second;
               prices->insert(order->price);
               bid_map[order->order_id] = order->bid;
               offset += 23;
               break;
            }
            case 'E':
            case 'C':
            case 'X':
               offset += 18;
               break;
            case 'D':
               offset += 14;
               break;
            case 'U': {
               auto order = (ItchOrderReplace *) &data[offset];
               if (order->new_order_id > max_order_id) max_order_id = order->new_order_id;
               auto pair = &stock_prices[order->stock_code];
               auto prices = bid_map[order->order_id] ? &pair->first : &pair->second;
               prices->insert(order->price);
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
            << "order count: " << order_cnt << std::endl
            << "elapsed: " << elap.count() << std::endl
            << "orders/sec: " << ops << std::endl
            << std::endl;

      return std::make_pair(max_order_id, stock_prices);
   }

   static auto SortMetaData(MAP<uint16_t, std::pair<SET<uint32_t>, SET<uint32_t>>> &stock_prices) {
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

   static void SaveMetaData(
         std::string &in_path,
         uint32_t max_order_id,
         std::map<uint16_t, std::pair<std::set<uint32_t>, std::set<uint32_t>>> &stock_prices) {
      auto base_fpath = RemoveExtension(in_path);
      std::ofstream out_bin(base_fpath + ".meta", std::ios_base::out | std::ios_base::binary);

      out_bin.write((char *) &max_order_id, sizeof(max_order_id));

      for (auto &[stock_code, pair]: stock_prices) {
         out_bin.write((const char *) &stock_code, sizeof(stock_code));
         WritePrices(out_bin, pair.first);
         WritePrices(out_bin, pair.second);
      }
   }

   static void WritePrices(std::ofstream &out, std::set<uint32_t> &prices) {
      auto ask_cnt = prices.size();
      out.write((char *) &ask_cnt, sizeof(ask_cnt));
      for (auto ask: prices) {
         out.write((char *) &ask, sizeof(ask));
      }

      if (out.fail()) {
         throw nostromo::Error("write() failed", EX_INFO);
      }
   }

   static void SetBaseFields(ItchBase &order, ItchRawBase &itch) {
      order.stock_code = StockCode(itch.stock_code);
      order.timestamp = Timestamp(itch.timestamp);
      order.order_id = OrderId(itch.order_id);
   }

   static void Read(
         boost::iostreams::filtering_istream &in,
         char *buf,
         std::streamsize n) {
      in.read(buf, n);
      if (in.gcount() != n) {
         throw nostromo::Error("read() failed", EX_INFO);
      }
   }

   static void Write(
         std::ofstream &out,
         char msg_type,
         void *obj,
         std::streamsize n) {
      out.write((char *) &msg_type, 1);
      out.write((char *) obj, n);
      if (out.fail()) {
         throw nostromo::Error("write() failed", EX_INFO);
      }
   }

   static uint16_t StockCode(__be16 stock_code) {
      return be16toh(stock_code);
   }

   static uint32_t OrderId(__be64 order_id) {
      return static_cast<uint32_t>(be64toh(order_id));
   }

   static uint32_t Quantity(__be32 quantity) {
      return be32toh(quantity);
   }

   static uint32_t Price(__be32 price) {
      return be32toh(price);
   }

   static uint64_t Timestamp(const __u8 *timestamp) {
      return ((uint64_t) (0) << 56) |
             ((uint64_t) (0) << 48) |
             ((uint64_t) (timestamp[0]) << 40) |
             ((uint64_t) (timestamp[1]) << 32) |
             ((uint64_t) (timestamp[2]) << 24) |
             ((uint64_t) (timestamp[3]) << 16) |
             ((uint64_t) (timestamp[4]) << 8) |
             ((uint64_t) (timestamp[5]));
   }

   static std::string RemoveExtension(std::string &path) {
      auto pos = path.find_last_of('.');
      if (pos <= 0) return path;
      return path.substr(0, pos);
   }

public:
   static void Run(std::string &fpath) {
      std::cout << "processing: " << fpath << std::endl;
      ParseRawFile(fpath);
      auto [max_order_id, stock_prices] = CreateMetaData(fpath);
      auto sorted = SortMetaData(stock_prices);
      SaveMetaData(fpath, max_order_id, sorted);
   }
};

} // namespace order_book::itch

int main() {
   using order_book::itch::ItchRawParser;

   std::cout.imbue(std::locale(""));
   std::string base_dir = "/remote/data/nasdaq-itch/";

   auto fnames = {
         "01302019.NASDAQ_ITCH50.gz",
         "01302020.NASDAQ_ITCH50.gz",
         "12302019.NASDAQ_ITCH50.gz"
   };

   for (auto &fname: fnames) {
      auto fpath = base_dir + fname;
      ItchRawParser::Run(fpath);
   }

   return 0;
}
