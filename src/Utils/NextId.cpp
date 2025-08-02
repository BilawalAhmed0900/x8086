#include "NextId.h"

size_t NextId() {
  static size_t id = 0;
  return id++;
}
