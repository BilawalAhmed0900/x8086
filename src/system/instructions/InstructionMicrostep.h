#pragma once

#include <functional>

#include "../CPU386.h"

using InstructionMicrostep = std::function<bool(CPU386&)>;
