#pragma once

#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace protei {

using json = nlohmann::json;

template <typename T>
T load_config(const std::string &path) {
  std::ifstream ifs(path);
  if (!ifs) throw std::runtime_error("Failed to open config: " + path);

  json j;
  ifs >> j;

  return j.get<T>();
}

}  // namespace protei
