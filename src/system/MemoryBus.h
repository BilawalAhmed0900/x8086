#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include "MemoryDevice.h"
#include <cstdint>

enum class BusControlLine {
  IDLE, READ, READ_SENT, WRITE, WRITE_SENT, READY
};

class MemoryBus {
 public:
  MemoryBus();
  void AddMemoryDevice(std::shared_ptr<MemoryDevice>&& device);

  bool read08(const uint32_t address);
  bool read16(const uint32_t address);
  bool read32(const uint32_t address);

  bool write08(const uint32_t address, uint8_t val);
  bool write16(const uint32_t address, uint16_t val);
  bool write32(const uint32_t address, uint32_t val);

  bool is_last_req_ready() const;
  bool clear_state();
  bool get_last_data(uint32_t& result) const;

  bool lock(const void* device);
  bool unlock(const void* device);

  void tick();
  void device_ticks();

 private:
  bool read(const uint32_t address, const uint32_t bits_needed);
  bool write(const uint32_t address, const uint32_t val, const uint32_t bits_needed);

  uint32_t address_line;
  BusControlLine control_line;
  uint32_t data_line;
  uint32_t bits_needed;

  std::vector<std::shared_ptr<MemoryDevice>> memory_devices;
  std::shared_ptr<MemoryDevice> last_requst_device;

  std::mutex mtx;
  size_t mtx_count;
  const void* owner;
};
