
#include "nostromo/mmap.hpp"

int main() {
   const std::string base_dir = "/remote/data/nasdaq-itch/";
   const std::string data_file = base_dir + "12302019.NASDAQ_ITCH50.bin";

   nostromo::Mmap<char> mmap{data_file};
   auto data = mmap.Ptr();
}