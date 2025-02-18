#ifndef ORDER_BOOK_UTILS_HPP
#define ORDER_BOOK_UTILS_HPP

#include <cstdint>
#include <string>

namespace order_book {

class Utils {
public:
   static auto Ops(const uint64_t nanos, const uint64_t count) {
      const auto nanos_d = static_cast<double>(nanos);
      const auto count_d = static_cast<double>(count);
      return static_cast<uint64_t>(count_d / (nanos_d / 1'000'000'000.0));
   }
};

} // namespace order_book

#endif //ORDER_BOOK_UTILS_HPP
