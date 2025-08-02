#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "InstructionMicrostep.h"

class CPU386;

class Instruction {
 public:
  Instruction(bool operand_size_override, bool address_size_override, bool lock,
              std::optional<uint16_t> segment_override);

  virtual bool step(CPU386& cpu);

 protected:
  bool operand_size_override;
  bool address_size_override;
  bool lock;
  std::optional<uint16_t> segment_override;

  std::vector<InstructionMicrostep> steps;
  std::vector<InstructionMicrostep>::const_iterator step_it;
};
