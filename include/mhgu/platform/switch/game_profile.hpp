#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "mhgu/core/types.hpp"

namespace mhgu::platform::switch_adapter {

constexpr std::size_t kBuildIdPrefixSize = 8;
using BuildIdPrefix = std::array<std::uint8_t, kBuildIdPrefixSize>;

struct MonsterLayout {
  std::uint32_t location_flag;
  std::uint8_t current_location_value;
  std::uint8_t remote_location_value;
  std::uint32_t secondary_identifier;
  std::uint32_t size_multiplier;
  std::uint32_t health;
  std::uint32_t maximum_health;
  std::uint32_t primary_identifier;
};

struct PointerListLayout {
  std::uint32_t marker;
  std::uint32_t padding;
  std::uint32_t pointers;
  std::uint32_t count;
  std::uint32_t byte_size;
};

struct FrameRatePatch {
  std::uint64_t pointer_from_main;
  std::uint64_t target_from_pointer;
  std::uint32_t fps30_value;
  std::uint32_t fps60_value;
};

struct MainWordPatch {
  std::uint64_t offset;
  std::uint32_t value;
};

constexpr std::size_t kMaxMainWordPatchesPerFeature = 8;

struct MainWordPatchSet {
  std::array<MainWordPatch, kMaxMainWordPatchesPerFeature> patches;
  std::size_t count;
};

struct GameProfile {
  const char* name;
  core::GameId game;
  std::uint64_t title_id;
  BuildIdPrefix build_id_prefix;
  std::uint64_t scan_start_from_heap;
  std::uint64_t scan_end_from_heap;
  MonsterLayout monster;
  PointerListLayout pointer_list;
  FrameRatePatch frame_rate;
  std::array<MainWordPatchSet, core::kRuntimeFeatureCount> runtime_patches;
};

constexpr std::uint64_t kMhguTitleId = 0x0100770008DD8000ULL;
constexpr std::uint64_t kMhxxTitleId = 0x0100C3800049C000ULL;
constexpr BuildIdPrefix kMhgu140BuildId{
  0xFB,
  0x08,
  0xF1,
  0xD2,
  0x0F,
  0xD1,
  0x20,
  0x4F,
};

const GameProfile* profile_for_process(
  std::uint64_t title_id,
  const std::uint8_t* build_id,
  std::size_t build_id_size
);

}  // namespace mhgu::platform::switch_adapter
