#include "MemoryDevice.h"

#include "../utils/NextId.h"

MemoryDevice::MemoryDevice() { id = NextId(); }
size_t MemoryDevice::getid() const { return id; }
