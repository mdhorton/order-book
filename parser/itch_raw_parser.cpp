#include "utils.hpp"
#include "itch/itch.hpp"
#include "itch/itch_raw.hpp"

#include "nostromo/error.hpp"
#include "nostromo/time_utils.hpp"

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include <fstream>
#include <iostream>

namespace order_book::itch {

// 1) read the compressed raw itch file
// 2) parse each message
// 3) skip the messages we don't need
// 4) skip the fields we don't need
// 5) write the messages in binary format
class ItchRawParser {
public:
   static void Run(const std::string &in_path) {
      std::cout << "processing: " << in_path << std::endl;

      namespace bio = boost::iostreams;

      const auto start = nostromo::TimeUtils::Now();

      const auto out_path = Utils::RemoveExtension(in_path) + ".bin";
      std::ofstream out_bin(out_path, std::ios_base::out | std::ios_base::binary);

      std::ifstream in(in_path, std::ios_base::in | std::ios_base::binary);
      bio::filtering_istream filter;
      filter.push(bio::gzip_decompressor()); // add decompressor to the filter stack.
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

      const auto stop = nostromo::TimeUtils::Now();
      const auto elap = stop - start;
      const auto elap_d = static_cast<double>(elap.count());
      const auto order_cnt_d = static_cast<double>(order_cnt);
      const auto ops = static_cast<uint64_t>(order_cnt_d / (elap_d / 1'000'000'000.0));

      std::cout
            << "order count: " << order_cnt << std::endl
            << "elapsed: " << elap.count() << std::endl
            << "orders/sec: " << ops << std::endl
            << std::endl;
   }

private:
   static void Handle_A(const char *msg, std::ofstream &bin_out) {
      const auto itch = (const ItchRawOrderAdd *) msg;
      ItchOrderAdd order{};
      SetBaseFields(order, *itch);
      order.bid = itch->bid == 'B' ? 1 : 0;
      order.quantity = Quantity(itch->quantity);
      order.price = Price(itch->price);
      WriteParsed(bin_out, 'A', &order, sizeof(ItchOrderAdd));
   }

   static void Handle_F(const char *msg, std::ofstream &out) {
      const auto itch = (const ItchRawOrderAddMpid *) msg;
      ItchOrderAdd order{};
      SetBaseFields(order, *itch);
      order.bid = itch->bid == 'B' ? 1 : 0;
      order.quantity = Quantity(itch->quantity);
      order.price = Price(itch->price);
      WriteParsed(out, 'F', &order, sizeof(ItchOrderAdd));
   }

   static void Handle_E(const char *msg, std::ofstream &out) {
      const auto itch = (const ItchRawOrderExecuted *) msg;
      ItchOrderExecuted order{};
      SetBaseFields(order, *itch);
      order.quantity = Quantity(itch->quantity);
      WriteParsed(out, 'E', &order, sizeof(ItchOrderExecuted));
   }

   static void Handle_C(const char *msg, std::ofstream &out) {
      const auto itch = (const ItchRawOrderExecutedPrice *) msg;
      ItchOrderExecuted order{};
      SetBaseFields(order, *itch);
      order.quantity = Quantity(itch->quantity);
      WriteParsed(out, 'C', &order, sizeof(ItchOrderExecuted));
   }

   static void Handle_X(const char *msg, std::ofstream &out) {
      const auto itch = (const ItchRawOrderCancel *) msg;
      ItchOrderCancel order{};
      SetBaseFields(order, *itch);
      order.quantity = Quantity(itch->quantity);
      WriteParsed(out, 'X', &order, sizeof(ItchOrderCancel));
   }

   static void Handle_D(const char *msg, std::ofstream &out) {
      const auto itch = (const ItchRawOrderDelete *) msg;
      ItchOrderDelete order{};
      SetBaseFields(order, *itch);
      WriteParsed(out, 'D', &order, sizeof(ItchOrderDelete));
   }

   static void Handle_U(const char *msg, std::ofstream &out) {
      const auto itch = (const ItchRawOrderReplace *) msg;
      ItchOrderReplace order{};
      SetBaseFields(order, *itch);
      order.new_order_id = OrderId(itch->new_order_id);
      order.quantity = Quantity(itch->quantity);
      order.price = Price(itch->price);
      WriteParsed(out, 'U', &order, sizeof(ItchOrderReplace));
   }

   static void SetBaseFields(ItchBase &order, const ItchRawBase &itch) {
      order.stock_code = StockCode(itch.stock_code);
      order.timestamp = Timestamp(itch.timestamp);
      order.order_id = OrderId(itch.order_id);
   }

   static void ReadRaw(
         boost::iostreams::filtering_istream &in,
         char *buf,
         const std::streamsize n) {
      in.read(buf, n);
      if (in.gcount() != n) {
         throw nostromo::Error("read() failed", EX_INFO);
      }
   }

   static void WriteParsed(
         std::ofstream &out,
         const char msg_type,
         const void *obj,
         const std::streamsize n) {
      out.write((const char *) &msg_type, sizeof(msg_type));
      out.write((const char *) obj, n);
   }

   static uint16_t StockCode(const __be16 stock_code) noexcept {
      return be16toh(stock_code);
   }

   static uint32_t OrderId(const __be64 order_id) noexcept {
      return static_cast<uint32_t>(be64toh(order_id));
   }

   static uint32_t Quantity(const __be32 quantity) noexcept {
      return be32toh(quantity);
   }

   static uint32_t Price(const __be32 price) noexcept {
      return be32toh(price);
   }

   static uint64_t Timestamp(const __u8 *timestamp) noexcept {
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
   std::cout.imbue(std::locale(""));
   const std::string base_dir = "/remote/data/nasdaq-itch/";

   const auto fnames = {
//         "01302019.NASDAQ_ITCH50.gz",
//         "01302020.NASDAQ_ITCH50.gz",
         "12302019.NASDAQ_ITCH50.gz"
   };

   for (const auto &fname: fnames) {
      order_book::itch::ItchRawParser::Run(base_dir + fname);
   }

   return 0;
}
