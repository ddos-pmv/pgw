#include <sys/socket.h>

#include <iostream>

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <IMSI>\n";
    std::exit(1);
  }

  return 0;
}