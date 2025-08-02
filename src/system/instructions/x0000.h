#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "../CPU386.h"
#include "Instruction.h"
#include "InstructionMicrostep.h"

class x0000 : public Instruction {
 public:
  x0000(bool operand_size_override, bool address_size_override, bool lock,
      std::optional<uint16_t> segment_override);

 private:
  uint32_t rm_byte;
  uint32_t displacement;
  uint16_t segment;
  uint32_t address;
  uint32_t mem8;
};
