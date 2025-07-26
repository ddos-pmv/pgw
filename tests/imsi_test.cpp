#include <gtest/gtest.h>
#include <pgw_utils/imsi.h>

using protei::IMSI;

TEST(IMSI_Test, ValidEncodingDecoding) {
  IMSI<> imsi("2089300007487");

  auto bcd = imsi.to_bcd();
  IMSI<> decoded = IMSI<>::from_bcd(bcd.data(), bcd.size());

  EXPECT_EQ(decoded.str(), "2089300007487");
}

TEST(IMSI_Test, MaxDigitsLimit) {
  // 15 digits - valid
  EXPECT_NO_THROW(IMSI<>("123456789012345"));

  // 16 digits - invalid
  EXPECT_THROW(IMSI<>("1234567890123456"), std::invalid_argument);
}

TEST(IMSI_Test, RejectsNonDigitInput) {
  EXPECT_THROW(IMSI<>("12345abc6789"), std::invalid_argument);
  EXPECT_THROW(IMSI<>(""), std::invalid_argument);
}

TEST(IMSI_Test, OddLengthEncoding) {
  IMSI<> imsi("12345");  // 5 digits -> 3 bytes

  auto bcd = imsi.to_bcd();
  EXPECT_EQ(bcd[0], 0x21);  // '1','2' → 0x21
  EXPECT_EQ(bcd[1], 0x43);  // '3','4' → 0x43
  EXPECT_EQ(bcd[2], 0x5F);  // '5','F' → 0x5F

  IMSI<> decoded = IMSI<>::from_bcd(bcd.data(), 3);
  EXPECT_EQ(decoded.str(), "12345");
}
