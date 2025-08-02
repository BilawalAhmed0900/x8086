#pragma once

#include <string>

#include "CPU386.h"
#include "IOBus.h"
#include "MemoryBus.h"

class System {
 public:
  System();
  void initialize(const std::string& bios_path);
  void run();

 private:
  MemoryBus memory_bus;
  IOBus io_bus;
  CPU386 cpu;
};
