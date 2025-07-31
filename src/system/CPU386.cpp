#include "CPU386.h"

#include <cstdint>
#include <memory>
#include <optional>

#include "../utils/Logger.h"
#include "IOBus.h"
#include "MemoryBus.h"
#include "instructions/Opcodes.h"
#include "instructions/x00.h"

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
      state = CPUStates::OPCODE_FETCH;
    }
  }
}

uint32_t CPU386::calculate_address(uint16_t segment, uint32_t address) const {
  if (CR0_PE) {
    return 0;
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

void CPU386::decode() {
  switch (opcode) {
    case ADD_RM_REG: {
      current_instruction = std::make_shared<x00>(
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
