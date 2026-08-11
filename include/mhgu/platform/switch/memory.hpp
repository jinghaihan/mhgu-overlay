#pragma once

#include <cstddef>
#include <cstdint>

namespace mhgu::platform::switch_adapter {

class MemoryAccess {
public:
  virtual ~MemoryAccess() = default;

  virtual bool pause() {
    return true;
  }

  virtual bool resume() {
    return true;
  }

  virtual bool
  read(std::uint64_t address, void* destination, std::size_t size) = 0;

  virtual bool
  write(std::uint64_t address, const void* source, std::size_t size) = 0;
};

}  // namespace mhgu::platform::switch_adapter
