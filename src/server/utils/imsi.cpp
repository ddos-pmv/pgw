// #include "imsi.h"

// #include <stdexcept>

// namespace protei {
// template <size_t MaxDigits>
// template <typename String>
// IMSI<MaxDigits>::IMSI(String&& imsi) {
//   std::string s = std::forward<String>(imsi);
//   if (imsi.empty() || imsi.size() > MaxDigits ||
//       imsi.find_first_not_of("1234567890") != std::string::npos) {
//     throw std::invalid_argument("Invalid IMSI string");
//   }

//   digits_ = std::move(s);
// }

// // template <size_t MaxDigits>
// // IMSI<MaxDigits>::IMSI(const std::string& imsi) {
// //   if (imsi.empty() || imsi.size() > MaxDigits ||
// //       imsi.find_first_not_of("1234567890")) {
// //     throw std::invalid_argument("Invalid IMSI string");
// //   }

// //   digits_ = imsi;
// // }

// // template <size_t MaxDigits>
// // IMSI<MaxDigits>::IMSI(std::string&& imsi) {
// //   if (imsi.empty() || imsi.size() > MaxDigits ||
// //       imsi.find_first_not_of("1234567890")) {
// //     throw std::invalid_argument("Invalid IMSI string");
// //   }

// //   digits_ = std::move(imsi);
// // }

// }  // namespace protei