#pragma once

#include <cstdint>
#include <vector>

#include "ROM.h"

class BIOSROM : public ROM {
 public:
  virtual ~BIOSROM() = default;
  BIOSROM(const std::vector<uint8_t>& data);

 public:
  /*
    Real system BIOS addresses in 8086, 65535 bytes
  */
  constexpr static size_t STARTING_ADDR = 0xF0000;
  constexpr static size_t ENDING_ADDR = 0x100000;  // end exclusive
  constexpr static size_t MAX_REAL_SIZE = ENDING_ADDR - STARTING_ADDR;

 private:
};
