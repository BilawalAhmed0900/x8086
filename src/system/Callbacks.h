#pragma once

#include <cstdint>
#include <functional>

using Read08Callback = std::function<void(const uint8_t val)>;
using Read16Callback = std::function<void(const uint16_t val)>;
using Read32Callback = std::function<void(const uint32_t val)>;

using Write08Callback = std::function<void()>;
using Write16Callback = std::function<void()>;
using Write32Callback = std::function<void()>;
