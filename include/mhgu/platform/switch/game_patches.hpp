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
  bool set_monster_damage_mode(core::MonsterDamageMode mode);
  bool enable_runtime_feature(core::RuntimeFeature feature);
  bool set_numeric_feature(core::NumericFeature feature, std::uint32_t value);

private:
  bool main_word_patch_address(
    const MainWordPatch& patch, std::uint64_t& address
  ) const;
  bool apply_main_word_patch(
    const MainWordPatch& patch, std::uint64_t address
  );
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
