#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace mhgu::core {

constexpr std::size_t kMaxMonsters = 10;

using MonsterId = std::uint16_t;
using MonsterHandle = std::uint64_t;

enum class GameId : std::uint8_t {
  Unknown,
  Mhgu,
  Mhxx,
};

enum class Locale : std::uint8_t {
  English,
  SimplifiedChinese,
  Japanese,
};

enum class LocaleMode : std::uint8_t {
  Auto,
  English,
  SimplifiedChinese,
  Japanese,
};

enum class Crown : std::uint8_t {
  None,
  Mini,
  Silver,
  Gold,
};

enum class SizePreset : std::uint8_t {
  Off,
  Mini,
  Silver,
  Gold,
};

enum class FrameRate : std::uint8_t {
  Fps30,
  Fps60,
};

enum class RuntimeFeature : std::uint8_t {
  MapAndLargeMonsters,
  CarryItemsIntoPouch,
  Invincible,
  HealthNoDecrease,
  StaminaNoDecrease,
  SharpnessNoDecrease,
  UnlockHunterArtSlots,
  UnlimitedHunterArts,
  ValorGaugeNoDecrease,
  AlchemyGaugeFull,
  SpStatusNoExpire,
  BowgunAutoReload,
  ConsumableItemsNoDecrease,
  WeaponTransmog,
  ArmorTransmog,
  Count,
};

enum class NumericFeature : std::uint8_t {
  HunterAffinity,
  Count,
};

constexpr std::size_t kRuntimeFeatureCount =
  static_cast<std::size_t>(RuntimeFeature::Count);

constexpr std::size_t runtime_feature_index(const RuntimeFeature feature) {
  return static_cast<std::size_t>(feature);
}

constexpr std::size_t kNumericFeatureCount =
  static_cast<std::size_t>(NumericFeature::Count);

constexpr std::size_t numeric_feature_index(const NumericFeature feature) {
  return static_cast<std::size_t>(feature);
}

struct NumericFeatureRange {
  std::uint16_t minimum;
  std::uint16_t maximum;
};

constexpr NumericFeatureRange numeric_feature_range(
  const NumericFeature feature
) {
  switch (feature) {
    case NumericFeature::HunterAffinity:
      return {0, 100};
    default:
      return {0, 0};
  }
}

struct NumericFeatureSetting {
  std::uint16_t value;
  bool enabled;
};

struct LocalizedNames {
  const char* english;
  const char* simplified_chinese;
  const char* japanese;
};

struct MonsterDefinition {
  MonsterId id;
  const char* key;
  LocalizedNames names;
  std::uint32_t base_size_x100;
  std::uint16_t mini_percent;
  std::uint16_t silver_percent;
  std::uint16_t gold_percent;
  std::uint16_t legal_min_percent;
  std::uint16_t legal_max_percent;
  bool variable_size;
};

struct MonsterSnapshot {
  MonsterHandle handle;
  MonsterId monster_id;
  std::uint32_t hp;
  std::uint32_t max_hp;
  std::uint16_t size_percent;
  bool hyper;
};

struct GameSnapshot {
  GameId game;
  Locale detected_locale;
  std::array<MonsterSnapshot, kMaxMonsters> monsters{};
  std::size_t monster_count{};
};

struct CoreSettings {
  LocaleMode locale_mode{LocaleMode::Auto};
  SizePreset size_preset{SizePreset::Off};
  FrameRate frame_rate{FrameRate::Fps30};
  std::array<bool, kRuntimeFeatureCount> runtime_features{};
  std::array<NumericFeatureSetting, kNumericFeatureCount> numeric_features{{
    {100, false},
  }};
};

struct MonsterView {
  MonsterHandle handle;
  MonsterId monster_id;
  const char* name;
  std::uint32_t hp;
  std::uint32_t max_hp;
  std::uint16_t hp_percent_x10;
  std::uint16_t size_percent;
  std::uint32_t actual_size_x100;
  Crown crown;
  bool hyper;
};

struct SizeWriteRequest {
  MonsterHandle handle;
  MonsterId monster_id;
  std::uint16_t target_percent;
};

struct CoreOutput {
  Locale locale{Locale::English};
  std::array<MonsterView, kMaxMonsters> monsters{};
  std::size_t monster_count{};
  std::array<SizeWriteRequest, kMaxMonsters> writes{};
  std::size_t write_count{};
};

}  // namespace mhgu::core
