#pragma once

#include <functional>

enum InstructionMicrostepResult {
  NOT_COMPLETED, COMPLETED, SKIPPED
};

class CPU386;

using InstructionMicrostep = std::function<InstructionMicrostepResult(CPU386&)>;
