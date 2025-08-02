#include "CPU386.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>

#include "../utils/Logger.h"
#include "IOBus.h"
#include "MemoryBus.h"
#include "instructions/Opcodes.h"
#include "instructions/x0000.h"

CPU386::CPU386(MemoryBus& memory_bus, IOBus& io_bus)
    : memory_bus(memory_bus), io_bus(io_bus) {
  reset();
}

void CPU386::reset() {
  EAX = ECX = EDX = EBX = ESP = EBP = ESI = EDI = 0;

  EFLAGS = 0;
  FLAGS_R1 = 1;

  CR0 = CR2 = CR3 = 0;
  CR0_ET = 1;
  CR0_NW = 1;
  CR0_CD = 1;
  CS = SS = DS = ES = FS = GS = 0;

  GDTR = {0, 0};

  /*
    According to Intel specs, at reset:
    - CS:IP = 0xFFFF:FFF0 (physical address 0xFFFFFFF0)
    - The CPU sets address lines A20-A31 high initially
    - On the first far jump, the CPU drops bits 20-31 to
      switch to the lower 1MB address space (where BIOS lives)

    This is quite complex to emulate exactly,
    so instead, we directly set CS:IP to 0xF000:0xFFF0,
    matching the BIOS address in the 1MB real-mode memory map.
  */
  CS = 0xF000;
  EIP = 0xFFF0;
  state = CPUStates::OPCODE_FETCH;
  MYLOG("Initial CPU state:");
  MYLOG("EAX : 0x%08X    EBX : 0x%08X    ECX: 0x%08X    EDX : 0x%08X", (int)EAX,
        (int)EBX, (int)ECX, (int)EDX);
  MYLOG("ESP : 0x%08X    EBP : 0x%08X    ESI: 0x%08X    EDI : 0x%08X", (int)ESP,
        (int)EBP, (int)ESI, (int)EDI);
  MYLOG("EFLAGS : 0x%08X", (int)EFLAGS);
  MYLOG("CR0 : 0x%08X    CR2 : 0x%08X    CR3 : 0x%08X", (int)CR0, (int)CR2,
        (int)CR3);
  MYLOG(
      "CS : 0x%08X    IP : 0x%08X    SS : 0x%08X    DS: 0x%08X    ES : 0x%08X  "
      "  FS :  0x%08X    GS : 0x%08X",
      (int)CS, (int)IP, (int)SS, (int)DS, (int)ES, (int)FS, (int)GS);
}

void CPU386::tick() {
  if (state == CPUStates::OPCODE_FETCH) {
    if (read08(CS, EIP)) {
      state = CPUStates::OPCODE_FETCHING;
    }
  } else if (state == CPUStates::OPCODE_FETCHING) {
    uint32_t val;
    if (!get_last_read(val)) {
      return;
    }
    if (val == Opcodes::OPERAND_SIZE_OVERRIDE) {
      operand_size_override = true;
      state = CPUStates::OPCODE_FETCH;
    } else if (val == Opcodes::ADDRESS_SIZE_OVERRIDE) {
      address_size_override = true;
      state = CPUStates::OPCODE_FETCH;
    } else if (val == Opcodes::LOCK) {
      lock = true;
      state = CPUStates::OPCODE_FETCH;
    } else if (val == Opcodes::ES_SEGMENT_OVERRIDE) {
      segment_override = ES;
      state = CPUStates::OPCODE_FETCH;
    } else if (val == Opcodes::CS_SEGMENT_OVERRIDE) {
      segment_override = CS;
      state = CPUStates::OPCODE_FETCH;
    } else if (val == Opcodes::SS_SEGMENT_OVERRIDE) {
      segment_override = SS;
      state = CPUStates::OPCODE_FETCH;
    } else if (val == Opcodes::DS_SEGMENT_OVERRIDE) {
      segment_override = DS;
      state = CPUStates::OPCODE_FETCH;
    } else if (val == Opcodes::FS_SEGMENT_OVERRIDE) {
      segment_override = FS;
      state = CPUStates::OPCODE_FETCH;
    } else if (val == Opcodes::GS_SEGMENT_OVERRIDE) {
      segment_override = GS;
      state = CPUStates::OPCODE_FETCH;
    } else {
      if (opcode == TWO_BYTE_INSTRUCTION) {
        opcode <<= 8;
        opcode |= static_cast<uint8_t>(val);
      } else {
        opcode = static_cast<uint8_t>(val);
      }

      if (opcode == TWO_BYTE_INSTRUCTION) {
        state = CPUStates::OPCODE_FETCH;
      } else {
        state = CPUStates::OPCODE_DECODE;
      }
    }
  } else if (state == CPUStates::OPCODE_DECODE) {
    decode();
  } else if (state == CPUStates::OPCODE_EXECUTE) {
    if (current_instruction->step(*this)) {
      current_instruction = nullptr;
      state = CPUStates::OPCODE_FETCH;
    }
  }
}

#define THROWS(x)
/*
  This function is one of the only functions throwing exceptions
  This is done to stop the instruction micro-step at that very instruction

  and come back to CPU.tick() gracefully to go into exception mode
*/
uint32_t CPU386::calculate_address(uint16_t segment, uint32_t address) const
    THROWS(CPUException) {
  if (CR0_PE) {
    /*
      Requested privilage level
    */

    constexpr size_t TI_BIT = 2;
    uint8_t RPL = segment & 0b11;
    uint8_t TI = (segment >> TI_BIT) & 0b1;  // 0 == GDTR, 1 == LDTR
    uint16_t index = (segment >> 3);

    /*
      Current privilage level
    */
    uint8_t CPL = CS & 0b11;

    /*
      Effective privilage level,
      we also need to take care into account
      the CS privilage level
    */
    uint8_t EPL = std::max(CPL, RPL);

    if ((TI == 0 && GDTR.descriptors.size() >= index) ||
        (TI == 1 && LDTR.descriptors.size() >= index)) {
      throw CPUException(GENERAL_PROTECTION, index << 3);
    }

    const auto& descriptor =
        (TI == 0) ? GDTR.descriptors.at(index) : LDTR.descriptors.at(index);

    constexpr size_t ACCESS_BIT_START = 5;  // 2-bits at this position
    constexpr size_t GRANULARITY_BIT = 3;
    uint8_t DPL = (descriptor.access >> ACCESS_BIT_START) & 0b11;
    if (EPL > DPL) {
      throw CPUException(GENERAL_PROTECTION, index << 3);
    }

    uint32_t limit = descriptor.limit;
    if ((descriptor.flags & (1 << GRANULARITY_BIT))) {
      /*
        Same as
          limit *= 4096
          limit += 4095

          if the granularity is set,
          we are in the 4KiB page limit
          not actual bytes, so real limit is not

          that amount of 4KiB pages
          += 4095 means to also include that
          whole page
      */
      limit = (limit << 12) | 0xFFF;
    }
    if (address > limit) {
      throw CPUException(GENERAL_PROTECTION, index << 3);
    }

    return descriptor.base + address;
  } else {
    return ((segment * 16) + address);
  }
}

bool CPU386::read08(uint16_t segment, uint32_t address) {
  if (!memory_bus.lock(this)) {
    return false;
  }
  if (!memory_bus.read08(calculate_address(segment, address))) {
    memory_bus.unlock(this);
    return false;
  }

  return true;
}

bool CPU386::read16(uint16_t segment, uint32_t address) {
  if (!memory_bus.lock(this)) {
    return false;
  }
  if (!memory_bus.read16(calculate_address(segment, address))) {
    memory_bus.unlock(this);
    return false;
  }

  return true;
}

bool CPU386::read32(uint16_t segment, uint32_t address) {
  if (!memory_bus.lock(this)) {
    return false;
  }
  if (!memory_bus.read32(calculate_address(segment, address))) {
    memory_bus.unlock(this);
    return false;
  }

  return true;
}

bool CPU386::write08(uint16_t segment, uint32_t address, uint8_t val) {
  if (!memory_bus.lock(this)) {
    return false;
  }
  if (!memory_bus.write08(calculate_address(segment, address), val)) {
    memory_bus.unlock(this);
    return false;
  }

  return true;
}

bool CPU386::write16(uint16_t segment, uint32_t address, uint16_t val) {
  if (!memory_bus.lock(this)) {
    return false;
  }
  if (!memory_bus.write16(calculate_address(segment, address), val)) {
    memory_bus.unlock(this);
    return false;
  }

  return true;
}

bool CPU386::write32(uint16_t segment, uint32_t address, uint32_t val) {
  if (!memory_bus.lock(this)) {
    return false;
  }
  if (!memory_bus.write32(calculate_address(segment, address), val)) {
    memory_bus.unlock(this);
    return false;
  }

  return true;
}

bool CPU386::get_last_write() {
  if (!memory_bus.is_last_req_ready()) {
    return false;
  }

  memory_bus.clear_state();
  memory_bus.unlock(this);
  return true;
}

bool CPU386::get_last_read(uint32_t& val) {
  if (!memory_bus.is_last_req_ready()) {
    return false;
  }

  if (!memory_bus.get_last_data(val)) {
    return false;
  }

  memory_bus.clear_state();
  memory_bus.unlock(this);
  return true;
}

bool CPU386::lock_bus() { return memory_bus.lock(this); }

bool CPU386::unlock_bus() { return memory_bus.unlock(this); }

uint32_t CPU386::IP_EIP() const { return CR0_PE ? EIP : IP; }

void CPU386::raise_exception(uint16_t exception_number, uint32_t error_code) {
  /*
    Implement later
  */
}

void CPU386::decode() {
  switch (opcode) {
    case ADD_RM_REG: {
      current_instruction = std::make_shared<x0000>(
          operand_size_override, address_size_override, lock, segment_override);
      break;
    }

    case HLT: {
      state = CPUStates::HALTED;
      break;
    }
  }

  operand_size_override = false;
  address_size_override = false;
  lock = false;
  segment_override = std::nullopt;
  opcode = 0xFFFF;
  if (state != CPUStates::HALTED) state = CPUStates::OPCODE_EXECUTE;
}

uint32_t CPU386::rm_byte_addtional_bytes_to_read(
    uint8_t rm_byte, bool address_size_override) const {
  const uint8_t mode = ((rm_byte >> 6) & 0b011);
  const uint8_t reg = ((rm_byte >> 3) & 0b111);
  const uint8_t r_m = ((rm_byte >> 0) & 0b111);

  if (mode == 0b11) {
    return 0;
  }

  /*
    In real mode
      - Default mode, 16-bit. address size override means 32-bit
    In protected mode
      - Default mode, 32-bit, address size override means 16-bit
  */
  const bool is_32_bit_read =
      (CR0_PE) ? !address_size_override : address_size_override;
  switch (r_m) {
    case 0b110: {
      /*
        Special case in (r_m == 0b110) and (mode == 0b00)
        direct displacement, for all other cases

        we can read upto 32 more bit as signed displacement
      */
      if (mode == 0b00) {
        return (is_32_bit_read) ? sizeof(uint32_t) : sizeof(uint16_t);
      } else if (mode == 0b01) {
        return sizeof(uint8_t);
      } else if (mode == 0b10) {
        return (is_32_bit_read) ? sizeof(uint32_t) : sizeof(uint16_t);
      } else {
        return 0;  // Should not happen
      }
    }
    default: {
      if (mode == 0b01) {
        return sizeof(uint8_t);
      } else if (mode == 0b10) {
        return (is_32_bit_read) ? sizeof(uint32_t) : sizeof(uint16_t);
      } else {
        return 0;
      }
    }
  }
}

/*
 *  Here mod 0b11 is not calculated, that is based upon instruction
 *  If the instruction is for reg8, then lower registers are used
 *  else whole 16-bit register is used
 *
 *  Detail: https://en.wikipedia.org/wiki/ModR/M
 *      16-bit mode
 *
 * These 16-bit displacement can become 32-bit in protected mode
 * or even in real-mode with `address_size_override`
 *
 *  R/M                       MOD
 *             00           01[a]             10          11
 *  000     [BX+SI]     [BX+SI+disp8]   [BX+SI+disp16]  AL / AX
 *  001     [BX+DI]     [BX+DI+disp8]   [BX+DI+disp16]  CL / CX
 *  010     [BP+SI]     [BP+SI+disp8]   [BP+SI+disp16]  DL / DX
 *  011     [BP+DI]     [BP+DI+disp8]   [BP+DI+disp16]  BL / BX
 *  100     [SI]        [SI+disp8]      [SI+disp16]     AH / SP
 *  101     [DI]        [DI+disp8]      [DI+disp16]     CH / BP
 *  110     [disp16]    [BP+disp8]      [BP+disp16]     DH / SI
 *  111     [BX]        [BX+disp8]      [BX+disp16]     BH / DI
 */
bool CPU386::get_address_rm_byte(
    const uint8_t mode, const uint8_t r_m, const uint32_t displacement,
    const std::optional<uint16_t>& segment_override, bool address_size_override,
    uint16_t& segment, uint32_t& address) const {
  segment = DS;

  /*
    In real mode
      - Default mode, 16-bit. address size override means 32-bit
    In protected mode
      - Default mode, 32-bit, address size override means 16-bit
  */
  const bool is_32_bit_read =
      (CR0_PE) ? !address_size_override : address_size_override;

  switch (r_m) {
    case 0b000:
      address = (is_32_bit_read) ? (EBX + ESI) : (BX + SI);
      break;
    case 0b001:
      address = (is_32_bit_read) ? (EBX + EDI) : (BX + DI);
      break;
    case 0b010:
      address = (is_32_bit_read) ? (EBP + ESI) : (BP + SI);
      segment = SS;
      break;
    case 0b011:
      address = (is_32_bit_read) ? (EBP + EDI) : (BP + DI);
      segment = SS;
      break;
    case 0b100:
      address = (is_32_bit_read) ? ESI : SI;
      break;
    case 0b101:
      address = (is_32_bit_read) ? EDI : DI;
      break;
    case 0b110: {
      if (mode == 0b01 || mode == 0b10) {
        address = (is_32_bit_read) ? EBP : BP;
        segment = SS;
        break;
      }
      if (mode == 0b00) {
        address = (is_32_bit_read) ? displacement
                                   : static_cast<uint16_t>(displacement);
        break;
      }
      break;
    }
    case 0b111:
      address = (is_32_bit_read) ? EBX : BX;
      break;
    default:
      return false;
  }
  if (mode == 0b01) {
    address += static_cast<int8_t>(static_cast<uint8_t>(displacement));
  } else if (mode == 0b10) {
    address += (is_32_bit_read)
                   ? static_cast<int32_t>(displacement)
                   : static_cast<int16_t>(static_cast<uint16_t>(displacement));
  }
  segment = segment_override.value_or(segment);
  return true;
}

bool CPU386::is_AF(const uint32_t lhs, const uint32_t rhs,
                   const uint64_t result) {
  // Auxiliary flag
  // A carry flag that is used to check if carry has happened
  // from left nibble to right nibble
  //
  //   |        |
  //   |-Left   |-Right
  // 0b0000     0000
  //
  // Instead of manually checking, used the formula from here
  // https://retrocomputing.stackexchange.com/questions/11262/can-someone-explain-this-algorithm-used-to-compute-the-auxiliary-carry-flag
  return ((static_cast<uint64_t>(lhs) ^ static_cast<uint64_t>(rhs) ^ result) &
          0x10ull) != 0;
}
