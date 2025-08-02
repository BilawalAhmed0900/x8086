#include "Instruction.h"

#include <cstdint>
#include <optional>

#include "../CPU386.h"
#include "InstructionMicrostep.h"

Instruction::Instruction(bool operand_size_override, bool address_size_override,
                         bool lock, std::optional<uint16_t> segment_override)
    : operand_size_override(operand_size_override),
      address_size_override(address_size_override),
      lock(lock),
      segment_override(segment_override) {}

bool Instruction::step(CPU386& cpu) {
  while (true) {
    /*
      Maybe due to skipping, we have finished the instruction
    */
    if (step_it == steps.cend()) break;
    const InstructionMicrostepResult result = (*step_it)(cpu);
    if (result == InstructionMicrostepResult::NOT_COMPLETED) break;
    if (result == InstructionMicrostepResult::COMPLETED) {
      ++step_it;
      break;
    }
    if (result == InstructionMicrostepResult::SKIPPED) {
      ++step_it;
    }
  }
  return step_it == steps.cend();
}
