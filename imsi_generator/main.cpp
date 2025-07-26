#include <iostream>
#include <random>
std::string generate_random_imsi() {
  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<> dist(0, 9);

  std::string imsi = "00101";  // MCC + MNC (пример)
  while (imsi.size() < 15) {
    imsi += std::to_string(dist(rng));
  }
  return imsi;
}

int main() {
  std::cout << generate_random_imsi() << std::endl;
  return 0;
}