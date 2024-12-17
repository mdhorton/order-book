#include <fstream>
#include <iostream>

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include "nostromo/mmap.hpp"
#include "nostromo/time_utils.hpp"

#include "itch/itch.hpp"
#include "itch/itch_raw.hpp"

namespace order_book::itch {

class ItchRawParser {
public:
   static void Parse(const std::string &fpath) {
      std::cout << "processing: " << fpath << std::endl;

      auto start = nostromo::TimeUtils::Now();
      auto order_cnt = fpath.ends_with(".gz") ?
                       ParseFileGzip(fpath) :
                       ParseFile(fpath);
      auto stop = nostromo::TimeUtils::Now();
      auto elap = stop - start;
      auto ops = order_cnt / (elap.count() / 1'000'000'000);

      std::cout
            << "order count: " << order_cnt << std::endl
            << "elapsed: " << elap.count() << std::endl
            << "orders/sec: " << ops << std::endl;
      std::cout << std::endl;
   }

   static uint64_t ParseFileGzip(const std::string &in_path) {
      namespace io = boost::iostreams;

      // replace .gz with .bin
      const auto out_path = in_path.substr(0, in_path.length() - 3) + ".bin";
      std::ofstream out(out_path, std::ios_base::out | std::ios_base::binary);

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
               Handle_A(msg, msg_type, out);
               ++order_cnt;
               break;
            }
            case 'F': {
               Read(filter, msg, 39);
               Handle_F(msg, msg_type, out);
               ++order_cnt;
               break;
            }
            case 'E': {
               Read(filter, msg, 30);
               Handle_E(msg, msg_type, out);
               ++order_cnt;
               break;
            }
            case 'C': {
               Read(filter, msg, 35);
               Handle_C(msg, msg_type, out);
               ++order_cnt;
               break;
            }
            case 'X': {
               Read(filter, msg, 22);
               Handle_X(msg, msg_type, out);
               ++order_cnt;
               break;
            }
            case 'D': {
               Read(filter, msg, 18);
               Handle_D(msg, msg_type, out);
               ++order_cnt;
               break;
            }
            case 'U': {
               Read(filter, msg, 34);
               Handle_U(msg, msg_type, out);
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

      return order_cnt;
   }

   static uint64_t ParseFile(const std::string &fpath) {
      std::ofstream out(fpath + ".bin", std::ios_base::out | std::ios_base::binary);

      nostromo::Mmap<char> mmap{fpath};
      auto data = mmap.Ptr();
      auto fsize = mmap.Size();
      uint64_t offset = 0;
      uint64_t order_cnt = 0;

      while (offset < fsize) {
         auto msg_type = data[offset++];

         switch (msg_type) {
            default:
               ++offset;
               break;
            case 'S':
               offset += 11;
               break;
            case 'R':
               offset += 38;
               break;
            case 'H':
               offset += 24;
               break;
            case 'Y':
               offset += 19;
               break;
            case 'L':
               offset += 25;
               break;
            case 'V':
               offset += 34;
               break;
            case 'W':
               offset += 11;
               break;
            case 'K':
               offset += 27;
               break;
            case 'J':
               offset += 34;
               break;
            case 'h':
               offset += 20;
               break;
            case 'A': {
               Handle_A(data + offset, msg_type, out);
               ++order_cnt;
               offset += 35;
               break;
            }
            case 'F': {
               Handle_F(data + offset, msg_type, out);
               ++order_cnt;
               offset += 39;
               break;
            }
            case 'E': {
               Handle_E(data + offset, msg_type, out);
               ++order_cnt;
               offset += 30;
               break;
            }
            case 'C': {
               Handle_C(data + offset, msg_type, out);
               ++order_cnt;
               offset += 35;
               break;
            }
            case 'X': {
               Handle_X(data + offset, msg_type, out);
               ++order_cnt;
               offset += 22;
               break;
            }
            case 'D': {
               Handle_D(data + offset, msg_type, out);
               ++order_cnt;
               offset += 18;
               break;
            }
            case 'U': {
               Handle_U(data + offset, msg_type, out);
               ++order_cnt;
               offset += 34;
               break;
            }
            case 'P':
               offset += 43;
               break;
            case 'Q':
               offset += 39;
               break;
            case 'B':
               offset += 18;
               break;
            case 'I':
               offset += 49;
               break;
            case 'N':
               offset += 19;
               break;
            case 'O':
               offset += 47;
               break;
         }
      }

      return order_cnt;
   }

   static void Handle_A(char *msg, char msg_type, std::ofstream &out) {
      auto itch = (ItchRawOrderAdd *) msg;
      ItchOrderAdd order{};
      order.stock_code = StockCode(itch->stock_code);
      order.timestamp = Timestamp(itch->timestamp);
      order.order_id = OrderId(itch->order_id);
      order.bid = itch->bid == 'B' ? 1 : 0;
      order.quantity = Quantity(itch->quantity);
      order.price = Price(itch->price);
      Write(out, msg_type, &order, sizeof(ItchOrderAdd));
   }

   static void Handle_F(char *msg, char msg_type, std::ofstream &out) {
      auto itch = (ItchRawOrderAddMpid *) msg;
      ItchOrderAdd order{};
      order.stock_code = StockCode(itch->stock_code);
      order.timestamp = Timestamp(itch->timestamp);
      order.order_id = OrderId(itch->order_id);
      order.bid = itch->bid == 'B' ? 1 : 0;
      order.quantity = Quantity(itch->quantity);
      order.price = Price(itch->price);
      Write(out, msg_type, &order, sizeof(ItchOrderAdd));
   }

   static void Handle_E(char *msg, char msg_type, std::ofstream &out) {
      auto itch = (ItchRawOrderExecuted *) msg;
      ItchOrderExecuted order{};
      order.stock_code = StockCode(itch->stock_code);
      order.timestamp = Timestamp(itch->timestamp);
      order.order_id = OrderId(itch->order_id);
      order.quantity = Quantity(itch->quantity);
      Write(out, msg_type, &order, sizeof(ItchOrderExecuted));
   }

   static void Handle_C(char *msg, char msg_type, std::ofstream &out) {
      auto itch = (ItchRawOrderExecutedPrice *) msg;
      ItchOrderExecuted order{};
      order.stock_code = StockCode(itch->stock_code);
      order.timestamp = Timestamp(itch->timestamp);
      order.order_id = OrderId(itch->order_id);
      order.quantity = Quantity(itch->quantity);
      Write(out, msg_type, &order, sizeof(ItchOrderExecuted));
   }

   static void Handle_X(char *msg, char msg_type, std::ofstream &out) {
      auto itch = (ItchRawOrderCancel *) msg;
      ItchOrderCancel order{};
      order.stock_code = StockCode(itch->stock_code);
      order.timestamp = Timestamp(itch->timestamp);
      order.order_id = OrderId(itch->order_id);
      order.quantity = Quantity(itch->quantity);
      Write(out, msg_type, &order, sizeof(ItchOrderCancel));
   }

   static void Handle_D(char *msg, char msg_type, std::ofstream &out) {
      auto itch = (ItchRawOrderDelete *) msg;
      ItchOrderDelete order{};
      order.stock_code = StockCode(itch->stock_code);
      order.timestamp = Timestamp(itch->timestamp);
      order.order_id = OrderId(itch->order_id);
      Write(out, msg_type, &order, sizeof(ItchOrderDelete));
   }

   static void Handle_U(char *msg, char msg_type, std::ofstream &out) {
      auto itch = (ItchRawOrderReplace *) msg;
      ItchOrderReplace order{};
      order.stock_code = StockCode(itch->stock_code);
      order.timestamp = Timestamp(itch->timestamp);
      order.orig_order_id = OrderId(itch->order_id);
      order.new_order_id = OrderId(itch->new_order_id);
      order.quantity = Quantity(itch->quantity);
      order.price = Price(itch->price);
      Write(out, msg_type, &order, sizeof(ItchOrderReplace));
   }

   static void Read(
         boost::iostreams::filtering_istream &in,
         char *buf,
         const std::streamsize n) {
      in.read(buf, n);
      if (in.gcount() != n) {
         throw nostromo::Error("read() failed", EX_INFO);
      }
   }

   static void Write(
         std::ofstream &out,
         const char msg_type,
         const void *obj,
         std::streamsize n) {
      out.write((const char *) &msg_type, 1);
      out.write((const char *) obj, n);
      if (out.fail()) {
         throw nostromo::Error("write() failed", EX_INFO);
      }
   }

   static uint16_t StockCode(const __be16 stock_code) {
      return be16toh(stock_code);
   }

   static uint32_t OrderId(const __be64 order_id) {
      return static_cast<uint32_t>(be64toh(order_id));
   }

   static uint32_t Quantity(const __be32 quantity) {
      return be32toh(quantity);
   }

   static uint32_t Price(const __be32 price) {
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
};

} // namespace order_book::itch

int main() {
   using order_book::itch::ItchRawParser;

   std::cout.imbue(std::locale(""));
   const std::string base_dir = "/remote/data/nasdaq-itch/";

   const auto fnames = {
         "01302019.NASDAQ_ITCH50.gz",
         "01302020.NASDAQ_ITCH50.gz",
         "12302019.NASDAQ_ITCH50.gz"
   };

   for (const auto &fname: fnames) {
      const auto fpath = base_dir + fname;
      ItchRawParser::Parse(fpath);
   }

   return 0;
}
