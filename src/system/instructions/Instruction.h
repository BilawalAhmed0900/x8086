#pragma once

#include <cstdint>
#include <optional>

class Instruction {
 public:
  Instruction(bool operand_size_override, bool address_size_override, bool lock,
              std::optional<uint16_t> segment_override);

 private:
  bool operand_size_override;
  bool address_size_override;
  bool lock;
  std::optional<uint16_t> segment_override;
};
