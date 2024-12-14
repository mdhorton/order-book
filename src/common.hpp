#ifndef ORDER_BOOK_COMMON_HPP
#define ORDER_BOOK_COMMON_HPP

#include <cstdint>
#include <cstdlib>
#include <stdexcept>

#define ALWAYS_INLINE inline __attribute__ ((__always_inline__))
#define PACKED __attribute__((packed))

namespace order_book {

class Utils {
public:
   template<class T>
   static T *Calloc(const size_t nmemb) {
      auto ptr = ::calloc(nmemb, sizeof(T));
      if (ptr == nullptr) {
         throw std::runtime_error("calloc() failed");
      }
      return static_cast<T *>(ptr);
   }
};

} //namespace order_book

#endif //ORDER_BOOK_COMMON_HPP
