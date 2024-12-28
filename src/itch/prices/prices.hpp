#ifndef ORDER_BOOK_PRICES_HPP
#define ORDER_BOOK_PRICES_HPP

#include <array>
#include <cstdint>

namespace order_book::itch::prices {

static constexpr std::array prices1{
      std::pair{1u, 1u},
      std::pair{2u, 1u}
};

static constexpr std::array prices2{
      std::pair{3u, 1u},
      std::pair{4u, 1u}
};

static constexpr std::array lookup{
      prices1, prices2
};

} // order_book::itch::prices

#endif //ORDER_BOOK_PRICES_HPP
