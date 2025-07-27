#include "ROM.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "../utils/Logger.h"
#include "Callbacks.h"
#include "MemoryDevice.h"

ROM::ROM(const uint32_t starting_addr, const uint32_t ending_addr,
         const std::vector<uint8_t>& data)
    : MemoryDevice(),
      starting_addr(starting_addr),
      ending_addr(ending_addr),
      control_line(RomControlSignal::IDLE),
      address_line(0),
      data_line(0) {
  if (starting_addr >= ending_addr) {
    MYLOG("Invalid starting address: %d, and ending address: %d",
          (int)starting_addr, (int)ending_addr);
    throw std::runtime_error("Invalid starting and ending address in ROM::ROM");
  }

  const uint32_t max_size = static_cast<int32_t>(ending_addr) -
                            static_cast<int32_t>(starting_addr) -
                            1 /* since end exclusive */;
  if (data.size() > max_size) {
    MYLOG("Invalid ROM size. Max size allowed: %d", (int)max_size);
    throw std::runtime_error("Invalid size for data in ROM::ROM");
  }

  const uint32_t base = max_size - static_cast<uint32_t>(data.size());
  this->data.resize(max_size);
  std::copy(data.begin(), data.end(), this->data.begin() + base);
}

bool ROM::owns(const uint32_t address) {
  MYLOG("ROM::owns called with address: %d", (int)address);
  return address >= starting_addr && address < ending_addr;
}

void ROM::tick() {
  if (control_line == RomControlSignal::IDLE) {
    MYLOG("Memory bus is idle, doing nothing");
    return;
  } else if (control_line == RomControlSignal::READ) {
    if (bits_needed & 0xFFFFFFFF) {
      const uint32_t result =
          *reinterpret_cast<uint32_t*>(&data[address_line - starting_addr]);
      data_line = result;
      control_line = RomControlSignal::READY;
    } else if (bits_needed & 0xFFFF) {
      const uint16_t result =
          *reinterpret_cast<uint16_t*>(&data[address_line - starting_addr]);
      data_line = result;
      control_line = RomControlSignal::READY;
    } else if (bits_needed & 0xFF) {
      const uint8_t result =
          *reinterpret_cast<uint8_t*>(&data[address_line - starting_addr]);
      data_line = result;
      control_line = RomControlSignal::READY;
    } else {
      MYLOG("READ call with invalid byte_needed: %d", (int)bits_needed);
    }
  } else if (control_line == RomControlSignal::WRITE) {
    if (bits_needed & 0xFFFFFFFF) {
      *reinterpret_cast<uint32_t*>(&data[address_line - starting_addr]) =
          static_cast<uint32_t>(data_line);
      control_line = RomControlSignal::READY;
    } else if (bits_needed & 0xFFFF) {
      *reinterpret_cast<uint16_t*>(&data[address_line - starting_addr]) =
          static_cast<uint16_t>(data_line);
      control_line = RomControlSignal::READY;
    } else if (bits_needed & 0xFF) {
      *reinterpret_cast<uint8_t*>(&data[address_line - starting_addr]) =
          static_cast<uint8_t>(data_line);
      control_line = RomControlSignal::READY;
    } else {
      MYLOG("WRITE call with invalid byte_needed: %d", (int)bits_needed);
    }
  } else if (control_line == RomControlSignal::READY) {
    MYLOG(
        "ROM served the request, waiting for device to read it and clear "
        "first");
  }
}

bool ROM::write08(const uint32_t address, const uint8_t val) {
  return write(address, val, 0xFF);
}

bool ROM::write16(const uint32_t address, const uint16_t val) {
  return write(address, val, 0xFFFF);
}

bool ROM::write32(const uint32_t address, const uint32_t val) {
  return write(address, val, 0xFFFFFFFF);
}

bool ROM::read08(const uint32_t address) { return read(address, 0xFF); }

bool ROM::read16(const uint32_t address) { return read(address, 0xFFFF); }

bool ROM::read32(const uint32_t address) { return read(address, 0xFFFFFFFF); }

bool ROM::is_last_req_ready() const {
  return control_line == RomControlSignal::READY;
}

bool ROM::clear_state() {
  control_line = RomControlSignal::IDLE;
  return true;
}

bool ROM::get_last_data(uint32_t& result) const {
  if (control_line != RomControlSignal::READY) {
    MYLOG("Trying to read data line even though it is not ready");
    return false;
  } else {
    result = data_line;
    return true;
  }
}

bool ROM::read(uint32_t address, uint32_t bits_needed) {
  MYLOG("ROM::read called with address: %d, bites_needed: %08x", (int)address,
        (int)bits_needed);
  this->control_line = RomControlSignal::READ;
  this->address_line = address;
  this->bits_needed = bits_needed;
  return true;
}

bool ROM::write(uint32_t address, uint32_t val, uint32_t bits_needed) {
  MYLOG("ROM::write called with address: %d, bites_needed: %08x", (int)address,
        (int)bits_needed);
  this->control_line = RomControlSignal::WRITE;
  this->data_line = val;
  this->address_line = address;
  this->bits_needed = bits_needed;
  return true;
}
