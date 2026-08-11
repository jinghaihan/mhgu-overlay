#pragma once

#include "mhgu/platform/switch/memory.hpp"

namespace mhgu::platform::switch_adapter {

class DmntMemoryAccess final : public MemoryAccess {
public:
  bool pause() override;
  bool resume() override;

  bool
  read(std::uint64_t address, void* destination, std::size_t size) override;

  bool
  write(std::uint64_t address, const void* source, std::size_t size) override;
};

}  // namespace mhgu::platform::switch_adapter
