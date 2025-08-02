#include "LoadFile.h"

#include <cstdint>
#include <exception>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

bool LoadFile(const std::string& file_path, std::vector<uint8_t>& data) {
  std::ifstream file(file_path, std::ios::binary);
  if (!file.is_open()) {
    return false;
  }

  if (!file.seekg(0, std::ios::end)) {
    return false;
  }
  const size_t filesize = static_cast<size_t>(file.tellg());
  if (!file.seekg(0, std::ios::beg)) {
    return false;
  }

  std::vector<uint8_t> result;

  try {
    result.resize(filesize);
  } catch (const std::bad_alloc&) {
    return false;
  }

  file.read(reinterpret_cast<char*>(result.data()),
            static_cast<std::streamsize>(filesize));
  if (file.gcount() != static_cast<std::streamsize>(filesize)) {
    return false;
  }
  data = std::move(result);
  return true;
}
