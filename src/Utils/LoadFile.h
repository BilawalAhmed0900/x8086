#pragma once

#include <cstdint>
#include <string>
#include <vector>

bool LoadFile(const std::string& file_path, std::vector<uint8_t>& data);
