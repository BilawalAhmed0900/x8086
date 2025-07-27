#pragma once

#include <cstdint>

class MemoryDevice {
 public:
  MemoryDevice();
  virtual ~MemoryDevice() = default;

  virtual bool owns(const uint32_t address) = 0;
  virtual void tick() = 0;

  virtual bool write08(const uint32_t address, const uint8_t val) = 0;
  virtual bool write16(const uint32_t address, const uint16_t val) = 0;
  virtual bool write32(const uint32_t address, const uint32_t val) = 0;

  virtual bool read08(const uint32_t address) = 0;
  virtual bool read16(const uint32_t address) = 0;
  virtual bool read32(const uint32_t address) = 0;

  virtual bool is_last_req_ready() const = 0;
  virtual bool clear_state() = 0;
  virtual bool get_last_data(uint32_t& result) const = 0;

  size_t getid() const;

 private:
  size_t id;
};
