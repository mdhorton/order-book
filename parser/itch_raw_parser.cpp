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
private:
   static void ParseRawFile(std::string &in_path) {
      namespace io = boost::iostreams;

      auto start = nostromo::TimeUtils::Now();

      auto out_path = RemoveExtension(in_path) + ".bin";
      std::ofstream out_bin(out_path, std::ios_base::out | std::ios_base::binary);

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
               ReadRaw(filter, msg, 11);
               break;
            case 'R':
               ReadRaw(filter, msg, 38);
               break;
            case 'H':
               ReadRaw(filter, msg, 24);
               break;
            case 'Y':
               ReadRaw(filter, msg, 19);
               break;
            case 'L':
               ReadRaw(filter, msg, 25);
               break;
            case 'V':
               ReadRaw(filter, msg, 34);
               break;
            case 'W':
               ReadRaw(filter, msg, 11);
               break;
            case 'K':
               ReadRaw(filter, msg, 27);
               break;
            case 'J':
               ReadRaw(filter, msg, 34);
               break;
            case 'h':
               ReadRaw(filter, msg, 20);
               break;
            case 'A': {
               ReadRaw(filter, msg, 35);
               Handle_A(msg, out_bin);
               ++order_cnt;
               break;
            }
            case 'F': {
               ReadRaw(filter, msg, 39);
               Handle_F(msg, out_bin);
               ++order_cnt;
               break;
            }
            case 'E': {
               ReadRaw(filter, msg, 30);
               Handle_E(msg, out_bin);
               ++order_cnt;
               break;
            }
            case 'C': {
               ReadRaw(filter, msg, 35);
               Handle_C(msg, out_bin);
               ++order_cnt;
               break;
            }
            case 'X': {
               ReadRaw(filter, msg, 22);
               Handle_X(msg, out_bin);
               ++order_cnt;
               break;
            }
            case 'D': {
               ReadRaw(filter, msg, 18);
               Handle_D(msg, out_bin);
               ++order_cnt;
               break;
            }
            case 'U': {
               ReadRaw(filter, msg, 34);
               Handle_U(msg, out_bin);
               ++order_cnt;
               break;
            }
            case 'P':
               ReadRaw(filter, msg, 43);
               break;
            case 'Q':
               ReadRaw(filter, msg, 39);
               break;
            case 'B':
               ReadRaw(filter, msg, 18);
               break;
            case 'I':
               ReadRaw(filter, msg, 49);
               break;
            case 'N':
               ReadRaw(filter, msg, 19);
               break;
            case 'O':
               ReadRaw(filter, msg, 47);
               break;
         }
      }

      if (out_bin.fail()) {
         throw nostromo::Error("write() failed", EX_INFO);
      }

      auto stop = nostromo::TimeUtils::Now();
      auto elap = stop - start;
      auto ops = static_cast<uint64_t>(
            static_cast<double>(order_cnt) /
            (static_cast<double>(elap.count()) / 1'000'000'000));

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
      WriteParsed(bin_out, 'A', &order, sizeof(ItchOrderAdd));
   }

   static void Handle_F(char *msg, std::ofstream &out) {
      auto itch = (ItchRawOrderAddMpid *) msg;
      ItchOrderAdd order{};
      SetBaseFields(order, *itch);
      order.bid = itch->bid == 'B' ? 1 : 0;
      order.quantity = Quantity(itch->quantity);
      order.price = Price(itch->price);
      WriteParsed(out, 'F', &order, sizeof(ItchOrderAdd));
   }

   static void Handle_E(char *msg, std::ofstream &out) {
      auto itch = (ItchRawOrderExecuted *) msg;
      ItchOrderExecuted order{};
      SetBaseFields(order, *itch);
      order.quantity = Quantity(itch->quantity);
      WriteParsed(out, 'E', &order, sizeof(ItchOrderExecuted));
   }

   static void Handle_C(char *msg, std::ofstream &out) {
      auto itch = (ItchRawOrderExecutedPrice *) msg;
      ItchOrderExecuted order{};
      SetBaseFields(order, *itch);
      order.quantity = Quantity(itch->quantity);
      WriteParsed(out, 'C', &order, sizeof(ItchOrderExecuted));
   }

   static void Handle_X(char *msg, std::ofstream &out) {
      auto itch = (ItchRawOrderCancel *) msg;
      ItchOrderCancel order{};
      SetBaseFields(order, *itch);
      order.quantity = Quantity(itch->quantity);
      WriteParsed(out, 'X', &order, sizeof(ItchOrderCancel));
   }

   static void Handle_D(char *msg, std::ofstream &out) {
      auto itch = (ItchRawOrderDelete *) msg;
      ItchOrderDelete order{};
      SetBaseFields(order, *itch);
      WriteParsed(out, 'D', &order, sizeof(ItchOrderDelete));
   }

   static void Handle_U(char *msg, std::ofstream &out) {
      auto itch = (ItchRawOrderReplace *) msg;
      ItchOrderReplace order{};
      SetBaseFields(order, *itch);
      order.new_order_id = OrderId(itch->new_order_id);
      order.quantity = Quantity(itch->quantity);
      order.price = Price(itch->price);
      WriteParsed(out, 'U', &order, sizeof(ItchOrderReplace));
   }

   static void SetBaseFields(ItchBase &order, ItchRawBase &itch) {
      order.stock_code = StockCode(itch.stock_code);
      order.timestamp = Timestamp(itch.timestamp);
      order.order_id = OrderId(itch.order_id);
   }

   static void ReadRaw(
         boost::iostreams::filtering_istream &in,
         char *buf,
         std::streamsize n) {
      in.read(buf, n);
      if (in.gcount() != n) {
         throw nostromo::Error("read() failed", EX_INFO);
      }
   }

   static void WriteParsed(
         std::ofstream &out,
         char msg_type,
         void *obj,
         std::streamsize n) {
      out.write((char *) &msg_type, sizeof(msg_type));
      out.write((char *) obj, n);
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
