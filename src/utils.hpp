#ifndef ORDER_BOOK_UTILS_HPP
#define ORDER_BOOK_UTILS_HPP

#include <cstdint>
#include <string>

namespace order_book {

class Utils {
public:
   static std::string RemoveExtension(const std::string &path) {
      const auto pos = path.find_last_of('.');
      if (pos <= 0) return path;
      return path.substr(0, pos);
   }

   static auto Ops(const uint64_t elap, const uint64_t count) {
      const auto elap_d = static_cast<double>(elap);
      const auto order_cnt_d = static_cast<double>(count);
      return static_cast<uint64_t>(order_cnt_d / (elap_d / 1'000'000'000.0));
   }
};

} // namespace order_book

#endif //ORDER_BOOK_UTILS_HPP
