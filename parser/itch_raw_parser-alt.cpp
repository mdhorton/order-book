#include <fstream>
#include <iostream>

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <variant>

#include "nostromo/mmap.hpp"
#include "nostromo/time_utils.hpp"

#include "itch/itch.hpp"
#include "itch/itch_raw.hpp"

namespace order_book::itch {

template<class... Ts>
struct Overloaded : Ts ... {
   using Ts::operator()...;
};

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

   static uint64_t ParseFileGzip(const std::string &fpath) {
      namespace io = boost::iostreams;

      // replace .gz with .bin
      const auto out_path = fpath.substr(0, fpath.length() - 3) + ".bin";
      std::ofstream out(out_path, std::ios_base::out | std::ios_base::binary);

      std::ifstream file(fpath, std::ios_base::in | std::ios_base::binary);
      io::filtering_istream in;
      in.push(io::gzip_decompressor());
      in.push(file);

      char msg_type;
      char msg[64]; // all messages are less than 64 bytes.

      uint64_t order_cnt = 0;

      auto handle_S = [&in, &msg]() { Read(in, msg, 11); };
      auto handle_R = [&in, &msg]() { Read(in, msg, 38); };
      auto handle_H = [&in, &msg]() { Read(in, msg, 24); };
      auto handle_Y = [&in, &msg]() { Read(in, msg, 19); };
      auto handle_L = [&in, &msg]() { Read(in, msg, 25); };
      auto handle_V = [&in, &msg]() { Read(in, msg, 34); };
      auto handle_W = [&in, &msg]() { Read(in, msg, 11); };
      auto handle_K = [&in, &msg]() { Read(in, msg, 27); };
      auto handle_J = [&in, &msg]() { Read(in, msg, 34); };
      auto handle_h = [&in, &msg]() { Read(in, msg, 20); };
      auto handle_P = [&in, &msg]() { Read(in, msg, 43); };
      auto handle_Q = [&in, &msg]() { Read(in, msg, 39); };
      auto handle_B = [&in, &msg]() { Read(in, msg, 18); };
      auto handle_I = [&in, &msg]() { Read(in, msg, 49); };
      auto handle_N = [&in, &msg]() { Read(in, msg, 19); };
      auto handle_O = [&in, &msg]() { Read(in, msg, 47); };
      auto handle_A = [&in, &msg, &msg_type, &out, &order_cnt]() {
         Read(in, msg, 35);
         Handle_A(msg, msg_type, out);
         ++order_cnt;
      };
      auto handle_F = [&in, &msg, &msg_type, &out, &order_cnt]() {
         Read(in, msg, 39);
         Handle_F(msg, msg_type, out);
         ++order_cnt;
      };
      auto handle_E = [&in, &msg, &msg_type, &out, &order_cnt]() {
         Read(in, msg, 30);
         Handle_E(msg, msg_type, out);
         ++order_cnt;
      };
      auto handle_C = [&in, &msg, &msg_type, &out, &order_cnt]() {
         Read(in, msg, 35);
         Handle_C(msg, msg_type, out);
         ++order_cnt;
      };
      auto handle_X = [&in, &msg, &msg_type, &out, &order_cnt]() {
         Read(in, msg, 22);
         Handle_X(msg, msg_type, out);
         ++order_cnt;
      };
      auto handle_D = [&in, &msg, &msg_type, &out, &order_cnt]() {
         Read(in, msg, 18);
         Handle_D(msg, msg_type, out);
         ++order_cnt;
      };
      auto handle_U = [&in, &msg, &msg_type, &out, &order_cnt]() {
         Read(in, msg, 34);
         Handle_U(msg, msg_type, out);
         ++order_cnt;
      };

      uintptr_t *handlers[128];

      handlers['A'] = (uintptr_t * ) & handle_A;

      void (*pf[128])(void *);
      pf['A'] = Handle_U;

      while (true) {
         in.read(&msg_type, 1);
         if (in.gcount() == 0) {
            if (in.eof()) break;
            throw nostromo::Error("read failed", EX_INFO);
         }

         if (msg_type == 'A') {
            auto foo = handlers[(int) msg_type];
            &foo();
         }
      }

      return order_cnt;
   }

   static uint64_t ParseFile(const std::string &fpath) {
      std::ofstream out(fpath + ".bin", std::ios_base::out | std::ios_base::binary);

      nostromo::Mmap<unsigned char> mmap{fpath};
      auto data = mmap.Ptr();
      auto fsize = mmap.Size();
      uint64_t offset = 0;
      uint64_t order_cnt = 0;

      while (offset < fsize) {
         auto msg_type = data[offset++];

         switch (msg_type) {
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
               auto itch = (ItchRawOrderAdd *) (data + offset);
               ItchOrderAdd order{};
               order.stock_code = StockCode(itch->stock_code);
               order.timestamp = Timestamp(itch->timestamp);
               order.order_id = OrderId(itch->order_id);
               order.bid = itch->bid == 'B' ? 1 : 0;
               order.quantity = Quantity(itch->quantity);
               order.price = Price(itch->price);
               Write(out, msg_type, &order, sizeof(ItchOrderAdd));
               ++order_cnt;
               offset += 35;
               break;
            }
            case 'F': {
               auto itch = (ItchRawOrderAddMpid *) (data + offset);
               ItchOrderAdd order{};
               order.stock_code = StockCode(itch->stock_code);
               order.timestamp = Timestamp(itch->timestamp);
               order.order_id = OrderId(itch->order_id);
               order.bid = itch->bid == 'B' ? 1 : 0;
               order.quantity = Quantity(itch->quantity);
               order.price = Price(itch->price);
               Write(out, msg_type, &order, sizeof(ItchOrderAdd));
               ++order_cnt;
               offset += 39;
               break;
            }
            case 'E': {
               auto itch = (ItchRawOrderExecuted *) (data + offset);
               ItchOrderExecuted order{};
               order.stock_code = StockCode(itch->stock_code);
               order.timestamp = Timestamp(itch->timestamp);
               order.order_id = OrderId(itch->order_id);
               order.quantity = Quantity(itch->quantity);
               Write(out, msg_type, &order, sizeof(ItchOrderExecuted));
               ++order_cnt;
               offset += 30;
               break;
            }
            case 'C': {
               auto itch = (ItchRawOrderExecutedPrice *) (data + offset);
               ItchOrderExecuted order{};
               order.stock_code = StockCode(itch->stock_code);
               order.timestamp = Timestamp(itch->timestamp);
               order.order_id = OrderId(itch->order_id);
               order.quantity = Quantity(itch->quantity);
               Write(out, msg_type, &order, sizeof(ItchOrderExecuted));
               ++order_cnt;
               offset += 35;
               break;
            }
            case 'X': {
               auto itch = (ItchRawOrderCancel *) (data + offset);
               ItchOrderCancel order{};
               order.stock_code = StockCode(itch->stock_code);
               order.timestamp = Timestamp(itch->timestamp);
               order.order_id = OrderId(itch->order_id);
               order.quantity = Quantity(itch->quantity);
               Write(out, msg_type, &order, sizeof(ItchOrderCancel));
               ++order_cnt;
               offset += 22;
               break;
            }
            case 'D': {
               auto itch = (ItchRawOrderDelete *) (data + offset);
               ItchOrderDelete order{};
               order.stock_code = StockCode(itch->stock_code);
               order.timestamp = Timestamp(itch->timestamp);
               order.order_id = OrderId(itch->order_id);
               Write(out, msg_type, &order, sizeof(ItchOrderDelete));
               ++order_cnt;
               offset += 18;
               break;
            }
            case 'U': {
               auto itch = (ItchRawOrderReplace *) (data + offset);
               ItchOrderReplace order{};
               order.stock_code = StockCode(itch->stock_code);
               order.timestamp = Timestamp(itch->timestamp);
               order.orig_order_id = OrderId(itch->order_id);
               order.new_order_id = OrderId(itch->new_order_id);
               order.quantity = Quantity(itch->quantity);
               order.price = Price(itch->price);
               Write(out, msg_type, &order, sizeof(ItchOrderReplace));
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
            default:
               ++offset;
         }
      }

      return order_cnt;
   }

   static void Handle_Z(void *ptr) {

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
         const unsigned char msg_type,
         const void *obj,
         std::streamsize n) {
      out.write((const char *) &msg_type, 1);
      if (out.fail()) {
         throw nostromo::Error("write() failed", EX_INFO);
      }
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
      return ((uint64_t)(0) << 56) |
             ((uint64_t)(0) << 48) |
             ((uint64_t)(timestamp[0]) << 40) |
             ((uint64_t)(timestamp[1]) << 32) |
             ((uint64_t)(timestamp[2]) << 24) |
             ((uint64_t)(timestamp[3]) << 16) |
             ((uint64_t)(timestamp[4]) << 8) |
             ((uint64_t)(timestamp[5]));
   }
};

} // namespace order_book::itch

int main() {
   std::cout.imbue(std::locale(""));
   const std::string base_dir = "/remote/data/nasdaq-itch/";

   const auto fnames = {
         "01302019.NASDAQ_ITCH50.gz",
         "01302020.NASDAQ_ITCH50.gz",
         "12302019.NASDAQ_ITCH50.gz"
   };

   for (const auto &fname: fnames) {
      const auto fpath = base_dir + fname;
      order_book::itch::ItchRawParser::Parse(fpath);
   }

   return 0;
}
