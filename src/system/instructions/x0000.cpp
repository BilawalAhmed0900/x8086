#include "x0000.h"

#include <cstdint>
#include <optional>

#include "../../utils/Logger.h"
#include "../CPU386.h"
#include "Instruction.h"
#include "InstructionMicrostep.h"

x0000::x0000(bool operand_size_override, bool address_size_override, bool lock,
             std::optional<uint16_t> segment_override)
    : Instruction(operand_size_override, address_size_override, lock,
                  segment_override),
      rm_byte(0),
      displacement(0),
      segment(0),
      address(0),
      mem8(0) {
  steps.push_back([this](CPU386& cpu) {
    if (!this->lock) return InstructionMicrostepResult::SKIPPED;
    MYLOG("x0000 locking the bus");
    return cpu.lock_bus() ? InstructionMicrostepResult::COMPLETED
                          : InstructionMicrostepResult::NOT_COMPLETED;
  });
  steps.push_back([this](CPU386& cpu) {
    MYLOG("x0000 attempting to read r/m byte");
    return cpu.read08(cpu.CS, cpu.IP_EIP())
               ? InstructionMicrostepResult::COMPLETED
               : InstructionMicrostepResult::NOT_COMPLETED;
  });
  steps.push_back([this](CPU386& cpu) {
    bool read_completed = cpu.get_last_read(rm_byte);
    MYLOG("x0000 read the r/m byte: 0x%08ullX", (unsigned long long)rm_byte);
    return read_completed ? InstructionMicrostepResult::COMPLETED
                          : InstructionMicrostepResult::NOT_COMPLETED;
  });
  steps.push_back([this](CPU386& cpu) {
    uint32_t bytes_needed = cpu.rm_byte_addtional_bytes_to_read(
        rm_byte, this->address_size_override);
    if (bytes_needed > 0) {
      MYLOG("x0000 addtional bytes needed: %d", (int)bytes_needed);
      switch (bytes_needed) {
        case sizeof(uint8_t): {
          return cpu.read08(cpu.CS, cpu.IP_EIP())
                     ? InstructionMicrostepResult::COMPLETED
                     : InstructionMicrostepResult::NOT_COMPLETED;
        }
        case sizeof(uint16_t): {
          return cpu.read16(cpu.CS, cpu.IP_EIP())
                     ? InstructionMicrostepResult::COMPLETED
                     : InstructionMicrostepResult::NOT_COMPLETED;
        }
        case sizeof(uint32_t): {
          return cpu.read32(cpu.CS, cpu.IP_EIP())
                     ? InstructionMicrostepResult::COMPLETED
                     : InstructionMicrostepResult::NOT_COMPLETED;
        }
        default: {
          throw CPUException(CPU386::INVALID_OPCODE, 0);
        }
      }
    }

    return InstructionMicrostepResult::SKIPPED;
  });
  steps.push_back([this](CPU386& cpu) {
    uint32_t bytes_needed = cpu.rm_byte_addtional_bytes_to_read(
        rm_byte, this->address_size_override);
    if (bytes_needed > 0) {
      bool read_completed = cpu.get_last_read(displacement);
      MYLOG("x0000 read the additional byte: 0x%08ullX",
            (unsigned long long)displacement);
      return read_completed ? InstructionMicrostepResult::COMPLETED
                            : InstructionMicrostepResult::NOT_COMPLETED;
    }
    return InstructionMicrostepResult::SKIPPED;
  });
  steps.push_back([this](CPU386& cpu) {
    const uint8_t mode = ((rm_byte >> 6) & 0b011);
    const uint8_t reg = ((rm_byte >> 3) & 0b111);
    const uint8_t r_m = ((rm_byte >> 0) & 0b111);

    MYLOG("x0000 r/m byte break, mode: 0x%01X, reg: 0x%01X, r_m: 0x%01X",
          (int)mode, (int)reg, (int)r_m);

    if (mode == 0b11) {
      const uint8_t lhs = *cpu.reg8[r_m];
      const uint8_t rhs = *cpu.reg8[reg];
      const uint16_t result =
          (static_cast<uint16_t>(lhs) + static_cast<uint16_t>(rhs));
      cpu.set_flags_add<sizeof(uint8_t)>(lhs, rhs, result);

      *cpu.reg8[r_m] = static_cast<uint8_t>(result);
      return InstructionMicrostepResult::COMPLETED;
    } else {
      if (!cpu.get_address_rm_byte(
              mode, r_m, displacement, this->segment_override,
              this->address_size_override, this->segment, this->address)) {
        throw CPUException(CPU386::INVALID_OPCODE, 0);
      }

      return cpu.read08(segment, address)
                 ? InstructionMicrostepResult::COMPLETED
                 : InstructionMicrostepResult::NOT_COMPLETED;
    }
  });
  steps.push_back([this](CPU386& cpu) {
    const uint8_t mode = ((rm_byte >> 6) & 0b011);
    const uint8_t reg = ((rm_byte >> 3) & 0b111);
    const uint8_t r_m = ((rm_byte >> 0) & 0b111);

    if (mode == 0b11) {
      return InstructionMicrostepResult::SKIPPED;
    } else {
      return cpu.get_last_read(mem8)
                 ? InstructionMicrostepResult::COMPLETED
                 : InstructionMicrostepResult::NOT_COMPLETED;
    }
  });
  steps.push_back([this](CPU386& cpu) {
    const uint8_t mode = ((rm_byte >> 6) & 0b011);
    const uint8_t reg = ((rm_byte >> 3) & 0b111);
    const uint8_t r_m = ((rm_byte >> 0) & 0b111);

    if (mode == 0b11) {
      return InstructionMicrostepResult::SKIPPED;
    } else {
      const uint8_t lhs = mem8;
      const uint8_t rhs = *cpu.reg8[reg];
      const uint16_t result =
          (static_cast<uint16_t>(lhs) + static_cast<uint16_t>(rhs));
      cpu.set_flags_add<sizeof(uint8_t)>(lhs, rhs, result);

      return cpu.write08(segment, address, static_cast<uint8_t>(result))
                 ? InstructionMicrostepResult::COMPLETED
                 : InstructionMicrostepResult::NOT_COMPLETED;
    }
  });
  steps.push_back([this](CPU386& cpu) {
    const uint8_t mode = ((rm_byte >> 6) & 0b011);
    const uint8_t reg = ((rm_byte >> 3) & 0b111);
    const uint8_t r_m = ((rm_byte >> 0) & 0b111);

    if (mode == 0b11) {
      return InstructionMicrostepResult::SKIPPED;
    } else {
      return cpu.get_last_write() ? InstructionMicrostepResult::COMPLETED
                                  : InstructionMicrostepResult::NOT_COMPLETED;
    }
  });
  steps.push_back([this](CPU386& cpu) {
    if (!this->lock) return InstructionMicrostepResult::SKIPPED;
    return cpu.unlock_bus() ? InstructionMicrostepResult::COMPLETED
                            : InstructionMicrostepResult::NOT_COMPLETED;
  });

  step_it = steps.cbegin();
}
