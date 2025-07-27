#pragma once

#include <string>

#include "CPU386.h"
#include "MemoryBus.h"
#include "IOBus.h"

class System {
 public:
  System();
  void Initialize(const std::string& bios_path);

 private:
  MemoryBus memory_bus;
  IOBus io_bus;
  CPU386 cpu;
};
