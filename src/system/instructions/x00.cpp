#include "x00.h"

#include <cstdint>
#include <optional>

#include "../CPU386.h"
#include "Instruction.h"

x00::x00(bool operand_size_override, bool address_size_override, bool lock,
         std::optional<uint16_t> segment_override)
    : Instruction(operand_size_override, address_size_override, lock,
                  segment_override) {}

bool x00::step(CPU386& cpu) { return true; }
