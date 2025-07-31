#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "../CPU386.h"
#include "Instruction.h"
#include "InstructionMicrostep.h"

class x00 : public Instruction {
 public:
  x00(bool operand_size_override, bool address_size_override, bool lock,
      std::optional<uint16_t> segment_override);
  bool step(CPU386& cpu) override;

 private:
  uint32_t rm_byte;
  uint32_t r8;
  std::vector<InstructionMicrostep> steps;
  std::vector<InstructionMicrostep>::const_iterator step_it;
};
