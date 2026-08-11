#pragma once

#include <cstddef>
#include <cstdint>

#include "mhgu/core/types.hpp"
#include "mhgu/platform/switch/game_profile.hpp"
#include "mhgu/platform/switch/memory.hpp"
#include "mhgu/platform/switch/patch_baseline.hpp"

namespace mhgu::platform::switch_adapter {

enum class QuestOperationResult : std::uint8_t {
  Success,
  NoActiveQuest,
  Failed,
};

class GamePatches {
public:
  GamePatches(
    MemoryAccess& memory,
    const GameProfile& profile,
    std::uint64_t main_base,
    std::uint64_t main_size,
    std::uint64_t address_space_base,
    std::uint64_t address_space_size,
    std::uint64_t heap_base = 0,
    std::uint64_t heap_size = 0
  );

  bool set_frame_rate(core::FrameRate frame_rate);
  bool set_monster_damage_mode(core::MonsterDamageMode mode);
  bool set_item_pouch_quantity(std::uint8_t slot, std::uint8_t quantity);
  QuestOperationResult maintain_quest(
    bool infinite_time, bool unlimited_faints
  );
  QuestOperationResult complete_quest();
  bool set_runtime_feature(core::RuntimeFeature feature, bool enabled);
  bool enable_runtime_feature(core::RuntimeFeature feature);
  bool set_numeric_feature(core::NumericFeature feature, std::uint32_t value);
  bool disable_numeric_feature(
    core::NumericFeature feature, std::uint32_t last_value
  );
  bool capture_baseline(
    const core::CoreSettings& settings, PatchBaseline& baseline
  );
  bool set_baseline(const PatchBaseline& baseline);

private:
  bool main_word_patch_address(
    const MainWordPatch& patch, std::uint64_t& address
  ) const;
  bool read_patch_words(
    const MainWordPatch* patches,
    std::size_t count,
    std::array<std::uint64_t, kMaxMainWordPatchesPerFeature>& addresses,
    std::array<std::uint32_t, kMaxMainWordPatchesPerFeature>& values
  );
  bool write_patch_words(
    const MainWordPatch* patches,
    const std::array<std::uint64_t, kMaxMainWordPatchesPerFeature>& addresses,
    const std::array<std::uint32_t, kMaxMainWordPatchesPerFeature>& previous,
    std::size_t count
  );
  bool baseline_patch_set(
    const MainWordPatch* patches,
    std::size_t count,
    std::array<MainWordPatch, kMaxMainWordPatchesPerFeature>& baseline
  ) const;
  QuestOperationResult quest_base(std::uint64_t& base);
  bool quest_address(
    std::uint64_t base, std::uint64_t offset, std::size_t size,
    std::uint64_t& address
  ) const;
  bool apply_value(
    std::uint64_t address, const void* value, std::size_t size
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
  std::uint64_t heap_base_;
  std::uint64_t heap_size_;
  PatchBaseline baseline_{};
  bool baseline_ready_{};
  std::array<
    std::array<MainWordPatch, kMaxMainWordPatchesPerFeature>,
    core::kNumericFeatureCount
  > active_numeric_patches_{};
  std::array<std::size_t, core::kNumericFeatureCount>
    active_numeric_patch_counts_{};
};

}  // namespace mhgu::platform::switch_adapter
