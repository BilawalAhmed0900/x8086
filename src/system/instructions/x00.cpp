#include "x00.h"

#include <cstdint>
#include <optional>

#include "../CPU386.h"
#include "Instruction.h"

x00::x00(bool operand_size_override, bool address_size_override, bool lock,
         std::optional<uint16_t> segment_override)
    : Instruction(operand_size_override, address_size_override, lock,
                  segment_override),
      rm_byte(0), r8(0) {
  steps.push_back([this](CPU386& cpu) {
    if (!this->lock) return true;
    return cpu.lock_bus();
  });
  steps.push_back([this](CPU386& cpu) {
    return cpu.read08(cpu.CS, cpu.EIP);
  });
  steps.push_back([this](CPU386& cpu) {
    return cpu.get_last_read(rm_byte);
  });
  steps.push_back([this](CPU386& cpu) {
    return cpu.read08(cpu.CS, cpu.EIP);
  });
  steps.push_back([this](CPU386& cpu) {
    return cpu.get_last_read(r8);
  });
  // ...
  steps.push_back([this](CPU386& cpu) {
    if (!this->lock) return true;
    return cpu.unlock_bus();
  });

  step_it = steps.cbegin();
}

bool x00::step(CPU386& cpu) {
  if ((*step_it)(cpu)) {
    ++step_it;
  }
  return step_it == steps.cend();
}
