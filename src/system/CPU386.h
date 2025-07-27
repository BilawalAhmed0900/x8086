#pragma once

#include <cstdint>
#include <optional>
#include <queue>

#include "IOBus.h"
#include "MemoryBus.h"

enum class CPUStates {
  OPCODE_FETCH,
  OPCODE_FETCHING,
  OPCODE_FETCHED,
  OPCODE_RUNNING,
  INTERRUPT_RUNNING,
  HALTED
};

#pragma pack(push, 1)
class CPU386 {
 public:
  CPU386(MemoryBus &memory_bus, IOBus &io_bus);
  void Reset();

  void tick();

  void read08();
  void read16();
  void read32();

 private:
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

  struct {
    uint32_t base;
    uint16_t limit;
  } GDTR;

  static constexpr size_t REG_COUNT = 8;
  uint32_t *reg32[REG_COUNT] = {&EAX, &ECX, &EDX, &EBX, &ESP, &EBP, &ESI, &EDI};
  uint16_t *reg16[REG_COUNT] = {&AX, &CX, &DX, &BX, &SP, &BP, &SI, &DI};
  uint8_t *reg8[REG_COUNT] = {&AL, &CL, &DL, &BL, &AH, &CH, &DH, &BH};

  uint16_t CS, SS, DS, ES;
  uint16_t FS, GS;

  MemoryBus &memory_bus;
  IOBus &io_bus;
  CPUStates state;

  bool operand_size_override;
  bool address_size_override;
  bool lock;
  std::optional<uint16_t> segment_override;
};
#pragma pack(pop)
