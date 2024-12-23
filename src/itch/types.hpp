#ifndef ORDER_BOOK_ITCH_TYPES_HPP
#define ORDER_BOOK_ITCH_TYPES_HPP

#include <map>
#include <set>

#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>

#define MAP std::unordered_map
#define SET std::unordered_set

namespace order_book::itch {

//template<typename K, typename V>
//using MAP = boost::unordered_flat_map<K, V>;

//template<typename K>
//using SET = boost::unordered_flat_set<K>;

} // order_book::itch

#endif //ORDER_BOOK_ITCH_TYPES_HPP
