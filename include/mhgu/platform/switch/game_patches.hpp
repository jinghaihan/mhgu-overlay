#pragma once

#include <cstddef>
#include <cstdint>

#include "mhgu/core/types.hpp"
#include "mhgu/platform/switch/game_profile.hpp"
#include "mhgu/platform/switch/memory.hpp"

namespace mhgu::platform::switch_adapter {

class GamePatches {
public:
  GamePatches(
    MemoryAccess& memory,
    const GameProfile& profile,
    std::uint64_t main_base,
    std::uint64_t main_size,
    std::uint64_t address_space_base,
    std::uint64_t address_space_size
  );

  bool set_frame_rate(core::FrameRate frame_rate);

private:
  bool contains(
    std::uint64_t region_base,
    std::uint64_t region_size,
    std::uint64_t address,
    std::size_t size
  ) const;

  MemoryAccess& memory_;
  const GameProfile& profile_;
  std::uint64_t main_base_;
  std::uint64_t main_size_;
  std::uint64_t address_space_base_;
  std::uint64_t address_space_size_;
};

}  // namespace mhgu::platform::switch_adapter
