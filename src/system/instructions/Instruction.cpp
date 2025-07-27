#include "Instruction.h"

#include <cstdint>
#include <optional>

Instruction::Instruction(bool operand_size_override, bool address_size_override,
                         bool lock, std::optional<uint16_t> segment_override)
    : operand_size_override(operand_size_override),
      address_size_override(address_size_override),
      lock(lock),
      segment_override(segment_override) {}
