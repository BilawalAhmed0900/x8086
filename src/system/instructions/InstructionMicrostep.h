#pragma once

#include <functional>

enum InstructionMicrostepResult {
  NOT_COMPLETED, COMPLETED, SKIPPED, EXCEPTION
};

class CPU386;

using InstructionMicrostep = std::function<InstructionMicrostepResult(CPU386&)>;
