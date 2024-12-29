#ifndef ORDER_BOOK_ITCH_METADATA_IO_HPP
#define ORDER_BOOK_ITCH_METADATA_IO_HPP

#include <cstdint>
#include <fstream>
#include <vector>
#include <map>

#include "nostromo/mmap.hpp"

#include "utils.hpp"

namespace order_book::itch {

class MetadataIO {
private:
   template<typename T>
   static auto ReadInt(std::ifstream &in) {
      T obj{};
      auto n = (std::streamsize) sizeof(T);
      in.read((char *) &obj, n);
      if (in.gcount() != n) {
         throw nostromo::Error("read() failed", EX_INFO);
      }
      return obj;
   }

   static auto ReadPrices(std::ifstream &in) {
      auto price_cnt = ReadInt<size_t>(in);

      std::vector<uint32_t> prices;
      prices.reserve(price_cnt);

      for (auto idx = 0u; idx < price_cnt; ++idx) {
         prices.emplace_back(ReadInt<uint32_t>(in));
      }

      return prices;
   }

   static void WritePrices(std::ofstream &out, auto &prices) {
      auto price_cnt = prices.size();
      out.write(reinterpret_cast<char *>(&price_cnt), sizeof(price_cnt));
      for (auto price: prices) {
         out.write(reinterpret_cast<char *>(&price), sizeof(price));
      }
   }

public:
   static auto Read(std::string &fpath) {
      auto in_path = Utils::RemoveExtension(fpath) + ".meta";
      std::ifstream in(in_path, std::ios_base::in | std::ios_base::binary);

      auto max_order_id = ReadInt<uint32_t>(in);
      auto max_stock_code = ReadInt<uint16_t>(in);
      auto stock_cnt = ReadInt<size_t>(in);

      // stock_code -> pair<bids, asks>
      std::map<uint16_t, std::pair<std::vector<uint32_t>, std::vector<uint32_t>>> stock_prices;

      for (auto idx = 0u; idx < stock_cnt; ++idx) {
         auto stock_code = ReadInt<uint16_t>(in);
         auto bids = ReadPrices(in);
         auto asks = ReadPrices(in);
         stock_prices[stock_code] = std::make_pair(bids, asks);
      }

      return std::make_tuple(max_order_id, max_stock_code, stock_prices);
   }

   static void Write(
         std::string &fpath,
         uint32_t max_order_id,
         uint16_t max_stock_code,
         auto &stock_prices) {
      auto out_path = Utils::RemoveExtension(fpath) + ".meta";
      std::ofstream out(out_path, std::ios_base::out | std::ios_base::binary);

      out.write(reinterpret_cast<char *>(&max_order_id), sizeof(max_order_id));
      out.write(reinterpret_cast<char *>(&max_stock_code), sizeof(max_stock_code));

      auto stock_cnt = stock_prices.size();
      out.write(reinterpret_cast<char *>(&stock_cnt), sizeof(stock_cnt));

      for (auto &[stock_code, pair]: stock_prices) {
         out.write(reinterpret_cast<const char *>(&stock_code), sizeof(stock_code));
         WritePrices(out, pair.first);
         WritePrices(out, pair.second);
      }

      if (out.fail()) {
         throw nostromo::Error("write() failed", EX_INFO);
      }
   }
};

} // namespace order_book::itch

#endif //ORDER_BOOK_ITCH_METADATA_IO_HPP
