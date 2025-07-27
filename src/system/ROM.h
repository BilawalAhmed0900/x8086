#pragma once

#include <cstdint>
#include <vector>

#include "Callbacks.h"
#include "MemoryDevice.h"

enum class RomControlSignal {
  IDLE, READ, WRITE, READY
};

class ROM : public MemoryDevice {
 public:
  ROM(const uint32_t starting_addr, const uint32_t ending_addr,
      const std::vector<uint8_t>& data);
  virtual ~ROM() = default;

  bool owns(const uint32_t address) override;
  void tick() override;

  bool write08(const uint32_t address, const uint8_t val) override;
  bool write16(const uint32_t address, const uint16_t val) override;
  bool write32(const uint32_t address, const uint32_t val) override;

  bool read08(const uint32_t address) override;
  bool read16(const uint32_t address) override;
  bool read32(const uint32_t address) override;

  bool is_last_req_ready() const override;
  bool clear_state() override;
  bool get_last_data(uint32_t& result) const override;

 private:
  bool read(uint32_t address, uint32_t bits_needed);
  bool write(uint32_t address, uint32_t val, uint32_t bits_needed);

  RomControlSignal control_line;
  uint32_t address_line;
  uint32_t data_line;
  uint32_t bits_needed;

  uint32_t starting_addr;
  uint32_t ending_addr;
  std::vector<uint8_t> data;
};
