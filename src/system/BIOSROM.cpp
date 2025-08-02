#include "BIOSROM.h"

#include <cstdint>
#include <vector>

#include "ROM.h"

BIOSROM::BIOSROM(const std::vector<uint8_t>& data)
    : ROM(STARTING_ADDR, ENDING_ADDR, data) {}
