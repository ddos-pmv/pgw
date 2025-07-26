#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace protei {

template <size_t MaxDigits = 15>
class IMSI {
 public:
  static constexpr size_t MaxBytes = (MaxDigits + 1) / 2;

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

  std::array<uint8_t, MaxBytes> to_bcd() const noexcept {
    std::array<uint8_t, MaxBytes> bcd{};
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

  static IMSI from_bcd(const uint8_t* data, size_t len) {
    std::string result;
    for (size_t i = 0; i < len; i++) {
      uint8_t second = data[i] & 0x0F;
      uint8_t first = (data[i] >> 4) & 0x0F;

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