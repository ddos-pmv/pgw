#include "pgw_server_app.h"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <path_to_config.json>\n";
    return 1;
  }

  try {
    protei::PGWServerApp app(argv[1]);
    app.run();
  } catch (const std::exception& e) {
    spdlog::error("Fatal error: {}", e.what());
    return 1;
  }
  return 0;
}
