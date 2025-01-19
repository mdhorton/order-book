#ifndef ORDER_BOOK_ITCH_USING_HPP
#define ORDER_BOOK_ITCH_USING_HPP

#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>

#include <cstdint>
#include <vector>
#include <map>

namespace order_book::itch {

template<typename K, typename V>
using UMAP = boost::unordered_flat_map<K, V>;

template<typename K>
using USET = boost::unordered_flat_set<K>;

// stock_code -> pair<asks, bids>
using STOCK_PRICE_UMAP = UMAP<uint16_t, std::pair<USET<uint32_t>, USET<uint32_t>>>;
using STOCK_PRICE_MAP = std::map<uint16_t, std::pair<std::vector<uint32_t>, std::vector<uint32_t>>>;

} //namespace order_book::itch

#endif //ORDER_BOOK_ITCH_USING_HPP
