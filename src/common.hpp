#ifndef ORDER_BOOK_COMMON_HPP
#define ORDER_BOOK_COMMON_HPP

#include <string>
#include <vector>

//#define ALWAYS_INLINE inline __attribute__ ((__always_inline__))
#define ALWAYS_INLINE inline
#define PACKED __attribute__((packed))

namespace order_book::itch {

const std::string DATA_DIR_BASE{"/remote/data/nasdaq-itch/"};

const std::vector<std::string> DATA_FILE_NAMES{
      // "01302019.NASDAQ_ITCH50",
      // "01302020.NASDAQ_ITCH50",
      "12302019.NASDAQ_ITCH50"
};

} //namespace order_book::itch

#endif //ORDER_BOOK_COMMON_HPP
