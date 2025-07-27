#pragma once

#include <cstdint>
#include <optional>

class CPU386;

class Instruction {
 public:
  Instruction(bool operand_size_override, bool address_size_override, bool lock,
              std::optional<uint16_t> segment_override);

  virtual bool step(CPU386& cpu) = 0;

 private:
  bool operand_size_override;
  bool address_size_override;
  bool lock;
  std::optional<uint16_t> segment_override;
};
