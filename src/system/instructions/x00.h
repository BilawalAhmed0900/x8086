#pragma once

#include <cstdint>
#include <optional>

#include "../CPU386.h"
#include "Instruction.h"

class x00 : public Instruction {
 public:
  x00(bool operand_size_override, bool address_size_override, bool lock,
      std::optional<uint16_t> segment_override);
  bool step(CPU386& cpu) override;
};
