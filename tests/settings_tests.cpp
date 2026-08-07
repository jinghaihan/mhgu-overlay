#include <cassert>
#include <cstdio>
#include <iostream>

#include "mhgu/app/settings.hpp"

int main() {
  using namespace mhgu;

  constexpr const char* kPath = "/tmp/mhgu-overlay-settings-tests.ini";
  std::remove(kPath);

  app::SettingsStore store(kPath);
  const auto defaults = store.load();
  assert(defaults.locale_mode == core::LocaleMode::Auto);
  assert(defaults.size_preset == core::SizePreset::Off);
  assert(defaults.frame_rate == core::FrameRate::Fps30);
  for (const auto enabled : defaults.runtime_features) {
    assert(!enabled);
  }
  const auto affinity_index = core::numeric_feature_index(
    core::NumericFeature::HunterAffinity
  );
  const auto palico_affinity_index = core::numeric_feature_index(
    core::NumericFeature::PalicoAffinity
  );
  const auto sp_level_index = core::numeric_feature_index(
    core::NumericFeature::SpLevel
  );
  const auto long_sword_spirit_index = core::numeric_feature_index(
    core::NumericFeature::LongSwordSpiritGauge
  );
  const auto attack_multiplier_index = core::numeric_feature_index(
    core::NumericFeature::AttackMultiplier
  );
  const auto defense_multiplier_index = core::numeric_feature_index(
    core::NumericFeature::DefenseMultiplier
  );
  const auto movement_speed_multiplier_index = core::numeric_feature_index(
    core::NumericFeature::MovementSpeedMultiplier
  );
  assert(defaults.numeric_features[affinity_index].value == 100);
  assert(!defaults.numeric_features[affinity_index].enabled);
  assert(defaults.numeric_features[palico_affinity_index].value == 100);
  assert(!defaults.numeric_features[palico_affinity_index].enabled);
  assert(defaults.numeric_features[sp_level_index].value == 4);
  assert(!defaults.numeric_features[sp_level_index].enabled);
  assert(defaults.numeric_features[long_sword_spirit_index].value == 100);
  assert(!defaults.numeric_features[long_sword_spirit_index].enabled);
  assert(defaults.numeric_features[attack_multiplier_index].value == 2);
  assert(!defaults.numeric_features[attack_multiplier_index].enabled);
  assert(defaults.numeric_features[defense_multiplier_index].value == 2);
  assert(!defaults.numeric_features[defense_multiplier_index].enabled);
  assert(
    defaults.numeric_features[movement_speed_multiplier_index].value == 20
  );
  assert(
    !defaults.numeric_features[movement_speed_multiplier_index].enabled
  );

  core::CoreSettings expected{};
  expected.locale_mode = core::LocaleMode::SimplifiedChinese;
  expected.size_preset = core::SizePreset::Gold;
  expected.frame_rate = core::FrameRate::Fps60;
  expected.runtime_features.fill(true);
  expected.numeric_features[affinity_index] = {73, true};
  expected.numeric_features[palico_affinity_index] = {61, true};
  expected.numeric_features[sp_level_index] = {3, true};
  expected.numeric_features[long_sword_spirit_index] = {77, true};
  expected.numeric_features[attack_multiplier_index] = {5, true};
  expected.numeric_features[defense_multiplier_index] = {5, true};
  expected.numeric_features[movement_speed_multiplier_index] = {25, true};
  assert(store.save(expected));

  const auto restored = store.load();
  assert(restored.locale_mode == expected.locale_mode);
  assert(restored.size_preset == expected.size_preset);
  assert(restored.frame_rate == expected.frame_rate);
  for (const auto enabled : restored.runtime_features) {
    assert(!enabled);
  }
  assert(restored.numeric_features[affinity_index].value == 73);
  assert(!restored.numeric_features[affinity_index].enabled);
  assert(restored.numeric_features[palico_affinity_index].value == 61);
  assert(!restored.numeric_features[palico_affinity_index].enabled);
  assert(restored.numeric_features[sp_level_index].value == 3);
  assert(!restored.numeric_features[sp_level_index].enabled);
  assert(restored.numeric_features[long_sword_spirit_index].value == 77);
  assert(!restored.numeric_features[long_sword_spirit_index].enabled);
  assert(restored.numeric_features[attack_multiplier_index].value == 5);
  assert(!restored.numeric_features[attack_multiplier_index].enabled);
  assert(restored.numeric_features[defense_multiplier_index].value == 5);
  assert(!restored.numeric_features[defense_multiplier_index].enabled);
  assert(
    restored.numeric_features[movement_speed_multiplier_index].value == 25
  );
  assert(
    !restored.numeric_features[movement_speed_multiplier_index].enabled
  );

  auto* numeric = std::fopen(kPath, "w");
  assert(numeric != nullptr);
  std::fprintf(numeric, "hunter_affinity=101\n");
  assert(std::fclose(numeric) == 0);
  assert(store.load().numeric_features[affinity_index].value == 100);

  numeric = std::fopen(kPath, "w");
  assert(numeric != nullptr);
  std::fprintf(numeric, "palico_affinity=42\n");
  assert(std::fclose(numeric) == 0);
  assert(store.load().numeric_features[palico_affinity_index].value == 42);

  numeric = std::fopen(kPath, "w");
  assert(numeric != nullptr);
  std::fprintf(numeric, "sp_level=5\n");
  assert(std::fclose(numeric) == 0);
  assert(store.load().numeric_features[sp_level_index].value == 4);

  numeric = std::fopen(kPath, "w");
  assert(numeric != nullptr);
  std::fprintf(numeric, "long_sword_spirit_gauge=77\n");
  assert(std::fclose(numeric) == 0);
  assert(
    store.load().numeric_features[long_sword_spirit_index].value == 77
  );

  numeric = std::fopen(kPath, "w");
  assert(numeric != nullptr);
  std::fprintf(numeric, "attack_multiplier=11\n");
  assert(std::fclose(numeric) == 0);
  assert(store.load().numeric_features[attack_multiplier_index].value == 10);

  numeric = std::fopen(kPath, "w");
  assert(numeric != nullptr);
  std::fprintf(numeric, "defense_multiplier=11\n");
  assert(std::fclose(numeric) == 0);
  assert(
    store.load().numeric_features[defense_multiplier_index].value == 10
  );

  numeric = std::fopen(kPath, "w");
  assert(numeric != nullptr);
  std::fprintf(numeric, "movement_speed_multiplier_x10=51\n");
  assert(std::fclose(numeric) == 0);
  assert(
    store.load().numeric_features[movement_speed_multiplier_index].value == 50
  );

  numeric = std::fopen(kPath, "w");
  assert(numeric != nullptr);
  std::fprintf(numeric, "hunter_affinity=-1\n");
  assert(std::fclose(numeric) == 0);
  assert(store.load().numeric_features[affinity_index].value == 0);

  numeric = std::fopen(kPath, "w");
  assert(numeric != nullptr);
  std::fprintf(numeric, "hunter_affinity=invalid\n");
  assert(std::fclose(numeric) == 0);
  assert(store.load().numeric_features[affinity_index].value == 100);

  auto* legacy = std::fopen(kPath, "w");
  assert(legacy != nullptr);
  std::fprintf(legacy, "size_preset=gold\nsize_lock=0\n");
  assert(std::fclose(legacy) == 0);
  assert(store.load().size_preset == core::SizePreset::Off);

  legacy = std::fopen(kPath, "w");
  assert(legacy != nullptr);
  std::fprintf(legacy, "size_preset=silver\nsize_lock=1\n");
  assert(std::fclose(legacy) == 0);
  assert(store.load().size_preset == core::SizePreset::Silver);

  std::remove(kPath);
  std::cout << "settings tests passed\n";
}
