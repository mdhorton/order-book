#ifndef ORDER_BOOK_UTILS_HPP
#define ORDER_BOOK_UTILS_HPP

#include <string>

namespace order_book {

class Utils {
public:
   static std::string RemoveExtension(const std::string &path) {
      const auto pos = path.find_last_of('.');
      if (pos <= 0) return path;
      return path.substr(0, pos);
   }
};

} // namespace order_book

#endif //ORDER_BOOK_UTILS_HPP
