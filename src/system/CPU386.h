#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "../utils/Logger.h"
#include "IOBus.h"
#include "MemoryBus.h"
#include "instructions/Instruction.h"

enum class CPUStates {
  OPCODE_FETCH,
  OPCODE_FETCHING,
  OPCODE_DECODE,
  OPCODE_EXECUTE,
  INTERRUPT,
  INTERRUPT_RUNNING,
  HALTED
};

class CPUException {
 public:
  CPUException(uint16_t exception_number, const std::optional<uint32_t>& error_code)
      : exception_number(exception_number), error_code(error_code) {}

 public:
  uint16_t exception_number;
  std::optional<uint32_t> error_code;
};

#pragma pack(push, 1)
class CPU386 {
 public:
  CPU386(MemoryBus &memory_bus, IOBus &io_bus);
  void reset();

  void tick();

  uint32_t calculate_address(uint16_t segment, uint32_t address) const;
  bool read08(uint16_t segment, uint32_t address);
  bool read16(uint16_t segment, uint32_t address);
  bool read32(uint16_t segment, uint32_t address);
  bool get_last_read(uint32_t &val);

  bool write08(uint16_t segment, uint32_t address, uint8_t val);
  bool write16(uint16_t segment, uint32_t address, uint16_t val);
  bool write32(uint16_t segment, uint32_t address, uint32_t val);
  bool get_last_write();

  bool lock_bus();
  bool unlock_bus();

  uint32_t IP_EIP() const;

  uint32_t rm_byte_addtional_bytes_to_read(uint8_t rm_byte,
                                           bool address_size_override) const;
  bool get_address_rm_byte(const uint8_t mode, const uint8_t r_m,
                           const uint32_t displacement,
                           const std::optional<uint16_t> &segment_override,
                           bool address_size_override, uint16_t &segment,
                           uint32_t &address) const;

  bool is_AF(const uint32_t lhs, const uint32_t rhs, const uint64_t result);

  template <size_t width>
  void adjust_flags(const uint64_t result);

  template <size_t width>
  void set_flags_add(const uint32_t lhs, const uint32_t rhs,
                     const uint64_t result);

  template <size_t width>
  void set_flags_sub(const uint32_t lhs, const uint32_t rhs,
                     const uint64_t result);

  template <size_t width>
  void set_flags_logical(const uint32_t lhs, const uint32_t rhs,
                         const uint64_t result);

  void raise_exception(uint16_t exception_number, uint32_t error_code);

 private:
  void decode();

 public:
  union {
    uint32_t EIP;
    struct {
      uint16_t IP;
      uint16_t ____IP_UNUSED;
    };
  };

  union {
    uint32_t EAX;
    struct {
      union {
        uint16_t AX;
        struct {
          uint8_t AL;
          uint8_t AH;
        };
      };
      uint16_t ____AX_UNUSED;
    };
  };

  union {
    uint32_t ECX;
    struct {
      union {
        uint16_t CX;
        struct {
          uint8_t CL;
          uint8_t CH;
        };
      };
      uint16_t ____CX_UNUSED;
    };
  };

  union {
    uint32_t EDX;
    struct {
      union {
        uint16_t DX;
        struct {
          uint8_t DL;
          uint8_t DH;
        };
      };
      uint16_t ____DX_UNUSED;
    };
  };

  union {
    uint32_t EBX;
    struct {
      union {
        uint16_t BX;
        struct {
          uint8_t BL;
          uint8_t BH;
        };
      };
      uint16_t ____BX_UNUSED;
    };
  };

  union {
    uint32_t ESP;
    struct {
      uint16_t SP;
      uint16_t ____SP_UNUSED;
    };
  };

  union {
    uint32_t EBP;
    struct {
      uint16_t BP;
      uint16_t ____BP_UNUSED;
    };
  };

  union {
    uint32_t ESI;
    struct {
      uint16_t SI;
      uint16_t ____SI_UNUSED;
    };
  };

  union {
    uint32_t EDI;
    struct {
      uint16_t DI;
      uint16_t ____DI_UNUSED;
    };
  };

  union {
    uint32_t EFLAGS;
    struct {
      union {
        uint16_t FLAGS;
        struct {
          uint16_t FLAGS_CF : 1;
          uint16_t FLAGS_R1 : 1;  // reserved, always 1
          uint16_t FLAGS_PF : 1;
          uint16_t FLAGS_R2 : 1;  // reserved, always 0
          uint16_t FLAGS_AF : 1;
          uint16_t FLAGS_R3 : 1;  // reserved, always 0
          uint16_t FLAGS_ZF : 1;
          uint16_t FLAGS_SF : 1;
          uint16_t FLAGS_TF : 1;
          uint16_t FLAGS_IF : 1;
          uint16_t FLAGS_DF : 1;
          uint16_t FLAGS_OF : 1;
          uint16_t FLAGS_IOPL : 2;
          uint16_t FLAGS_NT : 1;
          uint16_t FLAGS_R4 : 1;  // reserved, always 0
        };
      };

      union {
        uint16_t ____FLAGS_UNUSED;
        struct {
          uint16_t EFLAGS_RF : 1;
          uint16_t EFLAGS_VM : 1;
          uint16_t EFLAGS_AC : 1;
          uint16_t EFLAGS_VIF : 1;
          uint16_t EFLAGS_VIP : 1;
          uint16_t EFLAGS_ID : 1;
          uint16_t EFLAGS_R5 : 10;  // reserved, always 0
        };
      };
    };
  };

  union {
    uint32_t CR0;
    struct {
      uint32_t CR0_PE : 1;  // Protection Enable
      uint32_t CR0_MP : 1;  // Monitor Coprocessor
      uint32_t CR0_EM : 1;  // Emulation
      uint32_t CR0_TS : 1;  // Task Switched
      uint32_t
          CR0_ET : 1;  // Extension Type: always 1 on 80386 (387 FPU present)
      uint32_t CR0_NE : 1;  // Numeric Error
      uint32_t CR0_R1 : 10;
      uint32_t CR0_WP : 1;  // Write Protect
      uint32_t CR0_R2 : 1;
      uint32_t CR0_AM : 1;  // Alignment Mask
      uint32_t CR0_R3 : 10;
      uint32_t CR0_NW : 1;  // Not Write-through
      uint32_t CR0_CD : 1;  // Cache Disable
      uint32_t CR0_PG : 1;  // Paging
    };
  };

  uint32_t CR2;
  uint32_t CR3;

  struct Descriptor {
    uint32_t base;
    uint32_t limit;
    uint8_t access;
    uint8_t flags;
  };

  struct {
    uint32_t base;
    uint16_t limit;

    std::vector<Descriptor> descriptors;
  } GDTR;
  struct {
    uint32_t base;
    uint16_t limit;

    std::vector<Descriptor> descriptors;
  } LDTR;

  static constexpr size_t REG_COUNT = 8;
  uint32_t *reg32[REG_COUNT] = {&EAX, &ECX, &EDX, &EBX, &ESP, &EBP, &ESI, &EDI};
  uint16_t *reg16[REG_COUNT] = {&AX, &CX, &DX, &BX, &SP, &BP, &SI, &DI};
  uint8_t *reg8[REG_COUNT] = {&AL, &CL, &DL, &BL, &AH, &CH, &DH, &BH};

  uint16_t CS, SS, DS, ES;
  uint16_t FS, GS;

  static constexpr uint16_t INVALID_OPCODE = 6;
  static constexpr uint16_t SEGMENT_NOT_PRESENT = 11;
  static constexpr uint16_t GENERAL_PROTECTION = 13;

 private:
  MemoryBus &memory_bus;
  IOBus &io_bus;
  CPUStates state;

  bool operand_size_override;
  bool address_size_override;
  bool lock;
  std::optional<uint16_t> segment_override;
  uint16_t opcode;

  std::shared_ptr<Instruction> current_instruction;
};
#pragma pack(pop)

static uint8_t count_set_bits(const uint64_t num) {
  uint8_t count = 0;
  for (int i = 0; i < sizeof(num) * 8; i++) {
    count += (num >> i) & 1;
  }

  return count;
}

template <size_t width>
void CPU386::adjust_flags(const uint64_t result) {
  static_assert(width == sizeof(uint8_t) || width == sizeof(uint16_t) ||
                width == sizeof(uint32_t));

  MYLOG("adjust_flags called with byte: 0x%016ullX",
        (unsigned long long)result);

  // Since the maximum sum can only go maximum 1 bit ahead
  // e.g., 0xFF + 0xFF = 0x1FE,
  // Carry Flag
  const uint8_t CF = (result >> width) & 0x1;

  // Whole result is 0 or not, after addition, in the given width
  // e.g., 0x80 + 0x80 = 0x100, i.e. 0 is the 8 bit width
  // Zero Flag
  const uint8_t ZF = (result & ((1u << width) - 1)) == 0;

  // Left most digit in the width of the result is 1
  // Sign Flag
  const uint8_t SF = (result & (1u << (width - 1))) != 0;

  // Lowest 8 bit have even numbers of ones
  // Parity Flag
  const uint8_t PF = !(count_set_bits(result & 0xFF) & 1);

  this->FLAGS_CF = CF ? 1 : 0;
  this->FLAGS_ZF = ZF ? 1 : 0;
  this->FLAGS_SF = SF ? 1 : 0;
  this->FLAGS_PF = PF ? 1 : 0;

  MYLOG("Updating CF: 0x%01X", (int)CF);
  MYLOG("Updating ZF: 0x%01X", (int)ZF);
  MYLOG("Updating SF: 0x%01X", (int)SF);
  MYLOG("Updating PF: 0x%01X", (int)PF);
}

template <size_t width>
void CPU386::set_flags_add(const uint32_t lhs, const uint32_t rhs,
                           const uint64_t result) {
  static_assert(width == sizeof(uint8_t) || width == sizeof(uint16_t) ||
                width == sizeof(uint32_t));

  MYLOG(
      "set_flags_add called with lhs: 0x%016ullX, rhs: 0x%016ullX, result: "
      "0x%016ullX",
      (unsigned long long)lhs, (unsigned long long)rhs,
      (unsigned long long)result);

  const uint8_t AF = is_AF(lhs, rhs, result) ? 1 : 0;
  const uint8_t lhs_sign = (lhs & (1u << (width - 1))) != 0;
  const uint8_t rhs_sign = (rhs & (1u << (width - 1))) != 0;
  const uint8_t res_sign = (result & (1ull << (width - 1))) != 0;
  // Overflow Flag
  // Example: We went from a region of negativeness to positiveness
  // i.e., 0x80 - 0x1 = 0x7F that is positive if signess is concerned
  //       0x80 is negative
  //       0x01 is positive
  //       0x7F is positive
  const uint8_t OF = (lhs_sign == rhs_sign) && (lhs_sign != res_sign);

  this->FLAGS_AF = AF ? 1 : 0;
  this->FLAGS_OF = OF ? 1 : 0;

  MYLOG("Updating AF: 0x%01X", (int)AF);
  MYLOG("Updating OF: 0x%01X", (int)OF);

  adjust_flags<width>(result);
}

template <size_t width>
void CPU386::set_flags_sub(const uint32_t lhs, const uint32_t rhs,
                           const uint64_t result) {
  static_assert(width == sizeof(uint8_t) || width == sizeof(uint16_t) ||
                width == sizeof(uint32_t));

  MYLOG(
      "set_flags_sub called with lhs: 0x%016ullX, rhs: 0x%016ullX, result: "
      "0x%016ullX",
      (unsigned long long)lhs, (unsigned long long)rhs,
      (unsigned long long)result);

  const uint8_t AF = is_AF(lhs, rhs, result) ? 1 : 0;
  const uint8_t lhs_sign = (lhs & (1u << (width - 1))) != 0;
  const uint8_t rhs_sign = (rhs & (1u << (width - 1))) != 0;
  const uint8_t res_sign = (result & (1ull << (width - 1))) != 0;
  // Overflow Flag
  // Example: We went from a region of negativeness to positiveness
  // i.e., 0x80 - 0x1 = 0x7F that is positive if signess is concerned
  //       0x80 is negative
  //       0x01 is positive
  //       0x7F is positive
  const uint8_t OF = (lhs_sign != rhs_sign) && (lhs_sign != res_sign);

  this->FLAGS_AF = AF ? 1 : 0;
  this->FLAGS_OF = OF ? 1 : 0;

  MYLOG("Updating AF: 0x%01X", (int)AF);
  MYLOG("Updating OF: 0x%01X", (int)OF);

  adjust_flags<width>(result);
}

template <size_t width>
void CPU386::set_flags_logical(const uint32_t lhs, const uint32_t rhs,
                               const uint64_t result) {
  static_assert(width == sizeof(uint8_t) || width == sizeof(uint16_t) ||
                width == sizeof(uint32_t));

  MYLOG("set_flags_logical called with result: 0x%016ullX",
        (unsigned long long)result);

  /*
    This will already set to 0 in adjust_flags,
    but to be extra explicit clear
  */
  this->FLAGS_CF = 0;
  this->FLAGS_AF = 0;
  this->FLAGS_OF = 0;

  adjust_flags<width>(result);
}
