#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "mhgu/platform/switch/game_profile.hpp"

namespace mhgu::platform::switch_adapter {

constexpr std::size_t kMaxPatchBaselineEntries =
  1 +
  core::kRuntimeFeatureCount * kMaxMainWordPatchesPerFeature +
  core::kNumericFeatureCount * kMaxMainWordPatchesPerFeature;

struct PatchBaselineEntry {
  std::uint64_t offset{};
  std::uint32_t value{};
};

struct PatchBaseline {
  std::uint64_t title_id{};
  BuildIdPrefix build_id_prefix{};
  std::array<PatchBaselineEntry, kMaxPatchBaselineEntries> entries{};
  std::size_t count{};

  const PatchBaselineEntry* find(std::uint64_t offset) const;
};

std::size_t collect_patch_offsets(
  const GameProfile& profile,
  std::array<std::uint64_t, kMaxPatchBaselineEntries>& offsets
);

bool patch_baseline_matches_profile(
  const PatchBaseline& baseline, const GameProfile& profile
);

class PatchBaselineStore {
public:
  explicit PatchBaselineStore(std::string path);

  bool load(const GameProfile& profile, PatchBaseline& baseline) const;
  bool save(const GameProfile& profile, const PatchBaseline& baseline) const;

private:
  std::string path_;
};

}  // namespace mhgu::platform::switch_adapter
