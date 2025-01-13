#ifndef ORDER_BOOK_ITCH_METADATA_IO_HPP
#define ORDER_BOOK_ITCH_METADATA_IO_HPP

#include "utils.hpp"

#include "nostromo/error.hpp"

#include <cstdint>
#include <fstream>
#include <vector>
#include <map>
#include <set>

namespace order_book::itch {

class MetadataIO {
private:
   template<typename T>
   static auto ReadInt(std::ifstream &in) {
      T obj{};
      const auto n = (std::streamsize) sizeof(T);
      in.read((char *) &obj, n);
      if (in.gcount() != n) {
         throw nostromo::Error("read() failed", EX_INFO);
      }
      return obj;
   }

   static auto ReadPrices(std::ifstream &in) {
      const auto price_cnt = ReadInt<size_t>(in);

      std::vector<uint32_t> prices;
      prices.reserve(price_cnt);

      for (auto idx = 0u; idx < price_cnt; ++idx) {
         prices.emplace_back(ReadInt<uint32_t>(in));
      }

      return prices;
   }

   static void WritePrices(std::ofstream &out, const auto &prices) {
      const auto price_cnt = prices.size();
      out.write(reinterpret_cast<const char *>(&price_cnt), sizeof(price_cnt));
      for (const auto price: std::set<uint32_t>{prices.begin(), prices.end()}) {
         out.write(reinterpret_cast<const char *>(&price), sizeof(price));
      }
   }

public:
   static auto Read(const std::string &fpath) {
      const auto in_path = Utils::RemoveExtension(fpath) + ".meta";
      std::ifstream in(in_path, std::ios_base::in | std::ios_base::binary);

      const auto max_order_id = ReadInt<uint32_t>(in);
      const auto max_stock_code = ReadInt<uint16_t>(in);
      const auto stock_cnt = ReadInt<size_t>(in);

      // stock_code -> pair<bids, asks>
      std::map<uint16_t, std::pair<std::vector<uint32_t>, std::vector<uint32_t>>> stock_prices;

      for (auto idx = 0u; idx < stock_cnt; ++idx) {
         const auto stock_code = ReadInt<uint16_t>(in);
         const auto asks = ReadPrices(in);
         const auto bids = ReadPrices(in);
         stock_prices[stock_code] = std::make_pair(asks, bids);
      }

      return std::make_tuple(max_order_id, max_stock_code, stock_prices);
   }

   static void Write(
         const std::string &fpath,
         const uint32_t max_order_id,
         const uint16_t max_stock_code,
         const auto &stock_prices) {
      const auto out_path = Utils::RemoveExtension(fpath) + ".meta";
      std::ofstream out(out_path, std::ios_base::out | std::ios_base::binary);

      out.write(reinterpret_cast<const char *>(&max_order_id), sizeof(max_order_id));
      out.write(reinterpret_cast<const char *>(&max_stock_code), sizeof(max_stock_code));

      const auto stock_cnt = stock_prices.size();
      out.write(reinterpret_cast<const char *>(&stock_cnt), sizeof(stock_cnt));

      for (const auto &[stock_code, pair]: stock_prices) {
         out.write(reinterpret_cast<const char *>(&stock_code), sizeof(stock_code));
         const auto &[ask_prices, bid_prices] = pair;
         WritePrices(out, ask_prices);
         WritePrices(out, bid_prices);
      }

      if (out.fail()) {
         throw nostromo::Error("write() failed", EX_INFO);
      }
   }
};

} // namespace order_book::itch

#endif //ORDER_BOOK_ITCH_METADATA_IO_HPP
