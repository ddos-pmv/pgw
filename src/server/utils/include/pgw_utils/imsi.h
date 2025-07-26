#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace protei {

class IMSI {
 public:
  static constexpr size_t MaxBytes = 8;
  static constexpr size_t MaxDigits = 15;

  template <typename String>
    requires std::is_convertible_v<String, std::string>
  IMSI(String&& imsi) {
    std::string s = std::forward<String>(imsi);
    if (s.empty() || s.size() > MaxDigits ||
        s.find_first_not_of("1234567890") != std::string::npos) {
      throw std::invalid_argument("Invalid IMSI string");
    }

    digits_ = std::move(s);
  }

  //   IMSI(const std::string& imsi);
  //   explicit IMSI(std::string&& imsi);

  const std::string& str() const noexcept { return digits_; }

  std::vector<uint8_t> to_bcd() const noexcept {
    std::vector<uint8_t> bcd((digits_.size() + 1) / 2);
    size_t i = 0;
    for (; i + 1 < digits_.size(); i += 2) {
      uint8_t first = digits_[i] - '0';
      uint8_t second = digits_[i + 1] - '0';
      bcd[i / 2] = (second << 4) | first;
    }

    if (i < digits_.size()) {
      uint8_t second = digits_[i] - '0';
      bcd[i / 2] = (0xF << 4) | second;
    }

    return bcd;
  }

  static IMSI from_bcd(const uint8_t* data, size_t len) noexcept {
    std::string result;
    for (size_t i = 0; i < len; i++) {
      uint8_t second = data[i] & 0x0F;
      uint8_t first = (data[i] >> 4) & 0x0F;
      //   std::cerr << "i: " << i << std::endl;
      //   std::cout << "first: " << (char)(first + '0')
      //             << " second:" << (char)(second + '0') << std::endl;
      result += ('0' + second);
      if (first != 0xF) {
        result += ('0' + first);
      }
    }

    return IMSI(result);
  }

 private:
  std::string digits_;
};

}  // namespace protei