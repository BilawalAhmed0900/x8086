#include "MemoryBus.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>

#include "../utils/Logger.h"
#include "MemoryDevice.h"

MemoryBus::MemoryBus()
    : mtx(),
      mtx_count(0),
      owner(nullptr),
      address_line(0),
      control_line(BusControlLine::IDLE),
      data_line(0),
      bits_needed(0),
      last_requst_device(nullptr) {}

void MemoryBus::AddMemoryDevice(std::shared_ptr<MemoryDevice>&& device) {
  MYLOG("Connected device to bus with id: %d", (int)device->getid());
  this->memory_devices.push_back(device);
}

bool MemoryBus::read08(const uint32_t address) { return read(address, 0xFF); }

bool MemoryBus::read16(const uint32_t address) { return read(address, 0xFFFF); }

bool MemoryBus::read32(const uint32_t address) {
  return read(address, 0xFFFFFFFF);
}

bool MemoryBus::is_last_req_ready() const {
  return control_line == BusControlLine::READY;
}

bool MemoryBus::clear_state() {
  control_line = BusControlLine::IDLE;
  return true;
}

bool MemoryBus::get_last_data(uint32_t& result) const {
  if (owner == nullptr) {
    return false;
  }
  if (control_line != BusControlLine::READY) {
    MYLOG("Trying to read data line even though it is not ready by owner: %p",
          owner);
    return false;
  } else {
    result = data_line;
    return true;
  }
}

bool MemoryBus::lock(const void* device) {
  if (device == nullptr) {
    MYLOG("Empty device cannot call lock on bus");
    return false;
  }

  if (owner == nullptr) {
    if (!mtx.try_lock()) {
      MYLOG("Cannot lock the bus even when owner is nullptr");
      throw std::runtime_error("Cannot lock the bus");
    }
    mtx_count = 1;
    MYLOG("First locking call from owner: %p, new count: %d", device,
          (int)mtx_count);
    return true;
  } else if (device == owner) {
    ++mtx_count;
    MYLOG("Again locking call from owner: %p, new count: %d", device,
          (int)mtx_count);
    return true;
  } else {
    MYLOG("Locking call from device: %p, current owner: %p", device, owner,
          (int)mtx_count);
    return false;
  }
}

bool MemoryBus::unlock(const void* device) {
  if (device == nullptr) {
    MYLOG("Empty device cannot call unlock on bus");
    return false;
  }

  if (device == owner) {
    --mtx_count;
    if (mtx_count == 0) {
      mtx.unlock();
      owner = nullptr;
      MYLOG("Bus freed from current device: %p", device);
    } else {
      MYLOG("Unlock call from device: %p, new count: %d", device,
            (int)mtx_count);
    }
    return true;
  } else {
    MYLOG("Invalid unlock call from device: %p, current owner", device, owner);
    return false;
  }
}

void MemoryBus::tick() {
  if (control_line == BusControlLine::IDLE) {
    MYLOG("Memory bus is idle, doing nothing");
    return;
  } else if (control_line == BusControlLine::READ) {
    MYLOG(
        "Memory device asking each device for address ownership for READ "
        "request: %d",
        (int)address_line);
    for (auto& device : memory_devices) {
      if (device->owns(address_line)) {
        MYLOG("Device: %d owns the address: %d, serving the READ request to it",
              (int)device->getid(), (int)address_line);
        if (bits_needed & 0xFFFFFFFF) {
          device->read32(address_line);
          last_requst_device = device;
          control_line = BusControlLine::READ_SENT;
        } else if (bits_needed & 0xFFFF) {
          device->read16(address_line);
          last_requst_device = device;
          control_line = BusControlLine::READ_SENT;
        } else if (bits_needed & 0XFF) {
          device->read08(address_line);
          last_requst_device = device;
          control_line = BusControlLine::READ_SENT;
        } else {
          MYLOG("READ call with invalid byte_needed: %d", (int)bits_needed);
        }
        break;
      }
    }
  } else if (control_line == BusControlLine::WRITE) {
    MYLOG(
        "Memory device asking each device for address ownership for WRITE "
        "request: %d",
        (int)address_line);
    for (auto& device : memory_devices) {
      if (device->owns(address_line)) {
        MYLOG(
            "Device: %d owns the address: %d, serving the WRITE request to it",
            (int)device->getid(), (int)address_line);
        if (bits_needed & 0xFFFFFFFF) {
          device->write32(address_line, static_cast<uint32_t>(data_line));
          last_requst_device = device;
          control_line = BusControlLine::WRITE_SENT;
        } else if (bits_needed & 0xFFFF) {
          device->write16(address_line, static_cast<uint16_t>(data_line));
          last_requst_device = device;
          control_line = BusControlLine::WRITE_SENT;
        } else if (bits_needed & 0XFF) {
          device->write16(address_line, static_cast<uint8_t>(data_line));
          last_requst_device = device;
          control_line = BusControlLine::WRITE_SENT;
        } else {
          MYLOG("WRITE call with invalid byte_needed: %d", (int)bits_needed);
        }
        break;
      }
    }
  } else if (control_line == BusControlLine::READ_SENT) {
    if (last_requst_device->is_last_req_ready()) {
      uint32_t val;
      if (!last_requst_device->get_last_data(val)) {
        MYLOG("Device : %d is ready, but couldn't give the data",
              last_requst_device->getid());
        return;
      }

      data_line = val;
      control_line = BusControlLine::READY;

      last_requst_device->clear_state();
      last_requst_device = nullptr;
    }
  } else if (control_line == BusControlLine::WRITE_SENT) {
    if (last_requst_device->is_last_req_ready()) {
      control_line = BusControlLine::READY;
      last_requst_device->clear_state();
      last_requst_device = nullptr;
    }
  } else if (control_line == BusControlLine::READY) {
    MYLOG(
        "Bus served the request, waiting for device to read it and clear "
        "first");
  }
}

bool MemoryBus::read(const uint32_t address, const uint32_t bits_needed) {
  if (owner == nullptr) {
    MYLOG("READ request received by bus without owner, ignoring it");
    return false;
  } else if (control_line != BusControlLine::IDLE) {
    MYLOG(
        "READ request received by bus owner : %p without clearing it, ignoring "
        "it",
        owner);
    return false;
  } else {
    address_line = address;
    this->bits_needed = bits_needed;
    control_line = BusControlLine::READ;
    return true;
  }
}
