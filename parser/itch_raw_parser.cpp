#include "utils.hpp"
#include "itch/itch.hpp"
#include "itch/itch_raw.hpp"

#include "nostromo/error.hpp"
#include "nostromo/time_utils.hpp"

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include <fmt/format.h>

#include <fstream>

namespace order_book::itch {

// 1) read the orginal compressed raw itch file
// 2) parse each message
// 3) skip messages we don't need
// 4) skip fields we don't need
// 5) write the messages in binary format
class ItchRawParser {
public:
   static void Run(const std::string &fpath) {
      fmt::print("processing: {}\n", fpath);

      const auto start = nostromo::TimeUtils::Now();

      std::ifstream in(fpath + ".gz", std::ios_base::in | std::ios_base::binary);
      std::ofstream out(fpath + ".bin", std::ios_base::out | std::ios_base::binary);

      boost::iostreams::filtering_istream filter;
      filter.push(boost::iostreams::gzip_decompressor()); // add decompressor to the filter stack.
      filter.push(in); // add file stream to the filter stack.

      char msg_type;
      char msg[64]; // all itch messages are less than 64 bytes.
      uint64_t order_cnt = 0;

      while (true) {
         filter.read(&msg_type, 1);
         if (filter.gcount() == 0) {
            if (filter.eof()) {
               break;
            }
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
               Handle_A(msg, out);
               ++order_cnt;
               break;
            }
            case 'F': {
               ReadRaw(filter, msg, 39);
               Handle_F(msg, out);
               ++order_cnt;
               break;
            }
            case 'E': {
               ReadRaw(filter, msg, 30);
               Handle_E(msg, out);
               ++order_cnt;
               break;
            }
            case 'C': {
               ReadRaw(filter, msg, 35);
               Handle_C(msg, out);
               ++order_cnt;
               break;
            }
            case 'X': {
               ReadRaw(filter, msg, 22);
               Handle_X(msg, out);
               ++order_cnt;
               break;
            }
            case 'D': {
               ReadRaw(filter, msg, 18);
               Handle_D(msg, out);
               ++order_cnt;
               break;
            }
            case 'U': {
               ReadRaw(filter, msg, 34);
               Handle_U(msg, out);
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

      const auto elap = nostromo::TimeUtils::Now() - start;
      const auto ops = Utils::Ops(elap.count(), order_cnt);

      fmt::print("{}", fmt::format(
            std::locale("en_US.UTF-8"),
            "order count: {:L}\n"
            "elapsed: {:L} ns\n"
            "orders/sec: {:L}\n\n",
            order_cnt, elap.count(), ops
      ));
   }

private:
   static void Handle_A(const char *msg, std::ofstream &out) {
      const auto itch = reinterpret_cast<const ItchRawOrderAdd *>(msg);
      ItchOrderAdd order{};
      SetBaseFields(order, *itch);
      order.bid = itch->bid == 'B' ? 1 : 0;
      order.quantity = Quantity(itch->quantity);
      order.price = Price(itch->price);
      WriteParsed(out, 'A', &order, sizeof(ItchOrderAdd));
   }

   static void Handle_F(const char *msg, std::ofstream &out) {
      const auto itch = reinterpret_cast<const ItchRawOrderAddMpid *>(msg);
      ItchOrderAdd order{};
      SetBaseFields(order, *itch);
      order.bid = itch->bid == 'B' ? 1 : 0;
      order.quantity = Quantity(itch->quantity);
      order.price = Price(itch->price);
      WriteParsed(out, 'F', &order, sizeof(ItchOrderAdd));
   }

   static void Handle_E(const char *msg, std::ofstream &out) {
      const auto itch = reinterpret_cast<const ItchRawOrderExecuted *>(msg);
      ItchOrderExecuted order{};
      SetBaseFields(order, *itch);
      order.quantity = Quantity(itch->quantity);
      WriteParsed(out, 'E', &order, sizeof(ItchOrderExecuted));
   }

   static void Handle_C(const char *msg, std::ofstream &out) {
      const auto itch = reinterpret_cast<const ItchRawOrderExecutedPrice *>(msg);
      ItchOrderExecuted order{};
      SetBaseFields(order, *itch);
      order.quantity = Quantity(itch->quantity);
      WriteParsed(out, 'C', &order, sizeof(ItchOrderExecuted));
   }

   static void Handle_X(const char *msg, std::ofstream &out) {
      const auto itch = reinterpret_cast<const ItchRawOrderCancel *>(msg);
      ItchOrderCancel order{};
      SetBaseFields(order, *itch);
      order.quantity = Quantity(itch->quantity);
      assert(order.quantity != 0);
      WriteParsed(out, 'X', &order, sizeof(ItchOrderCancel));
   }

   static void Handle_D(const char *msg, std::ofstream &out) {
      const auto itch = reinterpret_cast<const ItchRawOrderDelete *>(msg);
      ItchOrderDelete order{};
      SetBaseFields(order, *itch);
      WriteParsed(out, 'D', &order, sizeof(ItchOrderDelete));
   }

   static void Handle_U(const char *msg, std::ofstream &out) {
      const auto itch = reinterpret_cast<const ItchRawOrderReplace *>(msg);
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
      out.write(reinterpret_cast<const char *>(&msg_type), sizeof(char));
      out.write(reinterpret_cast<const char *>(obj), n);

      if (out.fail()) {
         throw nostromo::Error("write() failed", EX_INFO);
      }
   }

   static uint16_t StockCode(const __be16 stock_code) noexcept {
      auto v = be16toh(stock_code);
      assert(v != 0);
      return v;
   }

   static uint32_t OrderId(const __be64 order_id) noexcept {
      auto v = static_cast<uint32_t>(be64toh(order_id));
      assert(v != 0);
      return v;
   }

   static uint32_t Quantity(const __be32 quantity) noexcept {
      auto v = be32toh(quantity);
      assert(v != 0);
      return v;
   }

   static uint32_t Price(const __be32 price) noexcept {
      auto v = be32toh(price);
      assert(v != 0);
      return v;
   }

   static uint64_t Timestamp(const __u8 *timestamp) noexcept {
      auto v =
            static_cast<uint64_t>(0) << 56 |
            static_cast<uint64_t>(0) << 48 |
            static_cast<uint64_t>(timestamp[0]) << 40 |
            static_cast<uint64_t>(timestamp[1]) << 32 |
            static_cast<uint64_t>(timestamp[2]) << 24 |
            static_cast<uint64_t>(timestamp[3]) << 16 |
            static_cast<uint64_t>(timestamp[4]) << 8 |
            static_cast<uint64_t>(timestamp[5]);
      assert(v != 0);
      return v;
   }
};

} // namespace order_book::itch

int main() {
   namespace itch = order_book::itch;

   for (const auto &fname: itch::DATA_FILE_NAMES) {
      const auto fpath = itch::DATA_DIR_BASE + fname;
      itch::ItchRawParser::Run(fpath);
   }

   return 0;
}
