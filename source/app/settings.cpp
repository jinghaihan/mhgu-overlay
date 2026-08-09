#include "mhgu/app/settings.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>

#ifdef __SWITCH__
#include <sys/stat.h>
#endif

namespace mhgu::app {
namespace {

constexpr std::array<const char*, core::kRuntimeFeatureCount>
  kRuntimeFeatureKeys{{
    "map_and_large_monsters",
    "carry_items_into_pouch",
    "invincible",
    "health_no_decrease",
    "stamina_no_decrease",
    "sharpness_no_decrease",
    "unlock_hunter_art_slots",
    "unlimited_hunter_arts",
    "valor_gauge_no_decrease",
    "alchemy_gauge_full",
    "sp_status_no_expire",
    "bowgun_auto_reload",
    "consumable_items_no_decrease",
    "weapon_transmog",
    "armor_transmog",
    "palico_health_no_decrease",
  }};

constexpr std::array<const char*, core::kNumericFeatureCount>
  kNumericFeatureEnabledKeys{{
    "hunter_affinity_enabled",
    "palico_affinity_enabled",
    "sp_level_enabled",
    "long_sword_spirit_gauge_enabled",
    "attack_multiplier_enabled",
    "defense_multiplier_enabled",
    "movement_speed_multiplier_enabled",
    "zenny_enabled",
    "wycademy_points_enabled",
  }};

core::LocaleMode parse_locale(const char* value) {
  if (std::strcmp(value, "en") == 0) {
    return core::LocaleMode::English;
  }
  if (std::strcmp(value, "zh-Hans") == 0) {
    return core::LocaleMode::SimplifiedChinese;
  }
  if (std::strcmp(value, "ja") == 0) {
    return core::LocaleMode::Japanese;
  }
  return core::LocaleMode::Auto;
}

core::SizePreset parse_preset(const char* value) {
  if (std::strcmp(value, "mini") == 0) {
    return core::SizePreset::Mini;
  }
  if (std::strcmp(value, "silver") == 0) {
    return core::SizePreset::Silver;
  }
  if (std::strcmp(value, "gold") == 0) {
    return core::SizePreset::Gold;
  }
  return core::SizePreset::Off;
}

core::FrameRate parse_frame_rate(const char* value) {
  return std::strcmp(value, "60") == 0 ? core::FrameRate::Fps60
                                       : core::FrameRate::Fps30;
}

core::HudLayout parse_hud_layout(const char* value) {
  if (std::strcmp(value, "top_right_vertical") == 0) {
    return core::HudLayout::TopRightVertical;
  }
  if (std::strcmp(value, "top_center_horizontal") == 0) {
    return core::HudLayout::TopCenterHorizontal;
  }
  return core::HudLayout::BottomLeftVertical;
}

core::MonsterDamageMode parse_monster_damage_mode(const char* value) {
  if (std::strcmp(value, "instant_kill") == 0) {
    return core::MonsterDamageMode::InstantKill;
  }
  if (std::strcmp(value, "leave_one_hp") == 0) {
    return core::MonsterDamageMode::LeaveOneHp;
  }
  return core::MonsterDamageMode::Off;
}

bool parse_enabled(const char* value) {
  return std::strcmp(value, "1") == 0;
}

std::uint16_t parse_percentage(const char* value) {
  char* end{};
  const auto parsed = std::strtol(value, &end, 10);
  if (end == value || *end != '\0') {
    return 100;
  }
  if (parsed < 0) {
    return 0;
  }
  if (parsed > 100) {
    return 100;
  }
  return static_cast<std::uint16_t>(parsed);
}

std::uint16_t parse_sp_level(const char* value) {
  char* end{};
  const auto parsed = std::strtol(value, &end, 10);
  if (end == value || *end != '\0') {
    return 4;
  }
  if (parsed < 1) {
    return 1;
  }
  if (parsed > 4) {
    return 4;
  }
  return static_cast<std::uint16_t>(parsed);
}

std::uint16_t parse_attack_multiplier(const char* value) {
  char* end{};
  const auto parsed = std::strtol(value, &end, 10);
  if (end == value || *end != '\0') {
    return 2;
  }
  if (parsed < 1) {
    return 1;
  }
  if (parsed > 10) {
    return 10;
  }
  return static_cast<std::uint16_t>(parsed);
}

std::uint16_t parse_defense_multiplier(const char* value) {
  char* end{};
  const auto parsed = std::strtol(value, &end, 10);
  if (end == value || *end != '\0') {
    return 2;
  }
  if (parsed < 1) {
    return 1;
  }
  if (parsed > 10) {
    return 10;
  }
  return static_cast<std::uint16_t>(parsed);
}

std::uint16_t parse_movement_speed_multiplier(const char* value) {
  char* end{};
  const auto parsed = std::strtol(value, &end, 10);
  if (end == value || *end != '\0') {
    return 20;
  }
  if (parsed < 10) {
    return 10;
  }
  if (parsed > 50) {
    return 50;
  }
  return static_cast<std::uint16_t>(parsed);
}

std::uint32_t parse_zenny(const char* value) {
  char* end{};
  const auto parsed = std::strtoll(value, &end, 10);
  if (end == value || *end != '\0') {
    return core::numeric_feature_range(core::NumericFeature::Zenny).maximum;
  }
  if (parsed < 0) {
    return 0;
  }
  if (parsed > 9999999) {
    return 9999999;
  }
  return static_cast<std::uint32_t>(parsed);
}

std::uint32_t parse_wycademy_points(const char* value) {
  char* end{};
  const auto parsed = std::strtoll(value, &end, 10);
  if (end == value || *end != '\0') {
    return core::numeric_feature_range(
      core::NumericFeature::WycademyPoints
    ).maximum;
  }
  if (parsed < 0) {
    return 0;
  }
  if (parsed > 9999999) {
    return 9999999;
  }
  return static_cast<std::uint32_t>(parsed);
}

std::uint8_t parse_item_pouch_slot(const char* value) {
  char* end{};
  const auto parsed = std::strtol(value, &end, 10);
  if (end == value || *end != '\0') {
    return 1;
  }
  return static_cast<std::uint8_t>(std::clamp(parsed, 1L, 10L));
}

std::uint8_t parse_item_pouch_quantity(const char* value) {
  char* end{};
  const auto parsed = std::strtol(value, &end, 10);
  if (end == value || *end != '\0') {
    return 99;
  }
  return static_cast<std::uint8_t>(std::clamp(parsed, 1L, 99L));
}

const char* locale_value(const core::LocaleMode mode) {
  switch (mode) {
    case core::LocaleMode::English:
      return "en";
    case core::LocaleMode::SimplifiedChinese:
      return "zh-Hans";
    case core::LocaleMode::Japanese:
      return "ja";
    default:
      return "auto";
  }
}

const char* preset_value(const core::SizePreset preset) {
  switch (preset) {
    case core::SizePreset::Mini:
      return "mini";
    case core::SizePreset::Silver:
      return "silver";
    case core::SizePreset::Gold:
      return "gold";
    default:
      return "off";
  }
}

const char* frame_rate_value(const core::FrameRate frame_rate) {
  return frame_rate == core::FrameRate::Fps60 ? "60" : "30";
}

const char* hud_layout_value(const core::HudLayout layout) {
  switch (layout) {
    case core::HudLayout::TopRightVertical:
      return "top_right_vertical";
    case core::HudLayout::TopCenterHorizontal:
      return "top_center_horizontal";
    default:
      return "bottom_left_vertical";
  }
}

const char* monster_damage_mode_value(const core::MonsterDamageMode mode) {
  switch (mode) {
    case core::MonsterDamageMode::InstantKill:
      return "instant_kill";
    case core::MonsterDamageMode::LeaveOneHp:
      return "leave_one_hp";
    default:
      return "off";
  }
}

}  // namespace

SettingsStore::SettingsStore(std::string path)
  : path_(std::move(path)) {}

core::CoreSettings SettingsStore::load() const {
  core::CoreSettings settings{};
  bool has_legacy_size_lock = false;
  bool legacy_size_lock_enabled = false;
  auto* file = std::fopen(path_.c_str(), "r");
  if (file == nullptr) {
    const auto backup = path_ + ".bak";
    file = std::fopen(backup.c_str(), "r");
  }
  if (file == nullptr) {
    return settings;
  }

  char line[128]{};
  while (std::fgets(line, sizeof(line), file) != nullptr) {
    char key[64]{};
    char value[64]{};
    if (std::sscanf(line, "%63[^=]=%63s", key, value) != 2) {
      continue;
    }
    if (std::strcmp(key, "language") == 0) {
      settings.locale_mode = parse_locale(value);
    } else if (std::strcmp(key, "size_preset") == 0) {
      settings.size_preset = parse_preset(value);
    } else if (std::strcmp(key, "frame_rate") == 0) {
      settings.frame_rate = parse_frame_rate(value);
    } else if (std::strcmp(key, "hud_layout") == 0) {
      settings.hud_layout = parse_hud_layout(value);
    } else if (std::strcmp(key, "damage_display") == 0) {
      settings.damage_display_enabled = parse_enabled(value);
    } else if (std::strcmp(key, "infinite_quest_time") == 0) {
      settings.infinite_quest_time = parse_enabled(value);
    } else if (std::strcmp(key, "unlimited_faints") == 0) {
      settings.unlimited_faints = parse_enabled(value);
    } else if (std::strcmp(key, "monster_damage_mode") == 0) {
      settings.monster_damage_mode = parse_monster_damage_mode(value);
    } else if (std::strcmp(key, "hunter_affinity") == 0) {
      settings.numeric_features[core::numeric_feature_index(
        core::NumericFeature::HunterAffinity
      )].value = parse_percentage(value);
    } else if (std::strcmp(key, "palico_affinity") == 0) {
      settings.numeric_features[core::numeric_feature_index(
        core::NumericFeature::PalicoAffinity
      )].value = parse_percentage(value);
    } else if (std::strcmp(key, "sp_level") == 0) {
      settings.numeric_features[core::numeric_feature_index(
        core::NumericFeature::SpLevel
      )].value = parse_sp_level(value);
    } else if (std::strcmp(key, "long_sword_spirit_gauge") == 0) {
      settings.numeric_features[core::numeric_feature_index(
        core::NumericFeature::LongSwordSpiritGauge
      )].value = parse_percentage(value);
    } else if (std::strcmp(key, "attack_multiplier") == 0) {
      settings.numeric_features[core::numeric_feature_index(
        core::NumericFeature::AttackMultiplier
      )].value = parse_attack_multiplier(value);
    } else if (std::strcmp(key, "defense_multiplier") == 0) {
      settings.numeric_features[core::numeric_feature_index(
        core::NumericFeature::DefenseMultiplier
      )].value = parse_defense_multiplier(value);
    } else if (std::strcmp(key, "movement_speed_multiplier_x10") == 0) {
      settings.numeric_features[core::numeric_feature_index(
        core::NumericFeature::MovementSpeedMultiplier
      )].value = parse_movement_speed_multiplier(value);
    } else if (std::strcmp(key, "zenny") == 0) {
      settings.numeric_features[core::numeric_feature_index(
        core::NumericFeature::Zenny
      )].value = parse_zenny(value);
    } else if (std::strcmp(key, "wycademy_points") == 0) {
      settings.numeric_features[core::numeric_feature_index(
        core::NumericFeature::WycademyPoints
      )].value = parse_wycademy_points(value);
    } else if (std::strcmp(key, "item_pouch_slot") == 0) {
      settings.item_pouch_slot = parse_item_pouch_slot(value);
    } else if (std::strcmp(key, "item_pouch_quantity") == 0) {
      settings.item_pouch_quantity = parse_item_pouch_quantity(value);
    } else if (std::strcmp(key, "size_lock") == 0) {
      has_legacy_size_lock = true;
      legacy_size_lock_enabled = parse_enabled(value);
    } else {
      for (std::size_t index = 0; index < kRuntimeFeatureKeys.size(); ++index) {
        if (std::strcmp(key, kRuntimeFeatureKeys[index]) == 0) {
          settings.runtime_features[index] = parse_enabled(value);
          break;
        }
      }
      for (
        std::size_t index = 0;
        index < kNumericFeatureEnabledKeys.size();
        ++index
      ) {
        if (std::strcmp(key, kNumericFeatureEnabledKeys[index]) == 0) {
          settings.numeric_features[index].enabled = parse_enabled(value);
          break;
        }
      }
    }
  }
  std::fclose(file);
  if (has_legacy_size_lock && !legacy_size_lock_enabled) {
    settings.size_preset = core::SizePreset::Off;
  }
  return settings;
}

bool SettingsStore::save(const core::CoreSettings& settings) const {
#ifdef __SWITCH__
  mkdir("sdmc:/config", 0777);
  mkdir("sdmc:/config/mhgu-overlay", 0777);
#endif
  const auto temporary = path_ + ".tmp";
  auto* file = std::fopen(temporary.c_str(), "w");
  if (file == nullptr) {
    return false;
  }
  std::fprintf(file, "language=%s\n", locale_value(settings.locale_mode));
  std::fprintf(file, "size_preset=%s\n", preset_value(settings.size_preset));
  std::fprintf(file, "frame_rate=%s\n", frame_rate_value(settings.frame_rate));
  std::fprintf(file, "hud_layout=%s\n", hud_layout_value(settings.hud_layout));
  std::fprintf(
    file,
    "damage_display=%u\n",
    settings.damage_display_enabled ? 1U : 0U
  );
  std::fprintf(
    file,
    "infinite_quest_time=%u\n",
    settings.infinite_quest_time ? 1U : 0U
  );
  std::fprintf(
    file,
    "unlimited_faints=%u\n",
    settings.unlimited_faints ? 1U : 0U
  );
  std::fprintf(
    file,
    "monster_damage_mode=%s\n",
    monster_damage_mode_value(settings.monster_damage_mode)
  );
  for (std::size_t index = 0; index < kRuntimeFeatureKeys.size(); ++index) {
    std::fprintf(
      file,
      "%s=%u\n",
      kRuntimeFeatureKeys[index],
      settings.runtime_features[index] ? 1U : 0U
    );
  }
  for (
    std::size_t index = 0;
    index < kNumericFeatureEnabledKeys.size();
    ++index
  ) {
    std::fprintf(
      file,
      "%s=%u\n",
      kNumericFeatureEnabledKeys[index],
      settings.numeric_features[index].enabled ? 1U : 0U
    );
  }
  std::fprintf(
    file,
    "hunter_affinity=%u\n",
    settings.numeric_features[core::numeric_feature_index(
      core::NumericFeature::HunterAffinity
    )].value
  );
  std::fprintf(
    file,
    "palico_affinity=%u\n",
    settings.numeric_features[core::numeric_feature_index(
      core::NumericFeature::PalicoAffinity
    )].value
  );
  std::fprintf(
    file,
    "sp_level=%u\n",
    settings.numeric_features[core::numeric_feature_index(
      core::NumericFeature::SpLevel
    )].value
  );
  std::fprintf(
    file,
    "long_sword_spirit_gauge=%u\n",
    settings.numeric_features[core::numeric_feature_index(
      core::NumericFeature::LongSwordSpiritGauge
    )].value
  );
  std::fprintf(
    file,
    "attack_multiplier=%u\n",
    settings.numeric_features[core::numeric_feature_index(
      core::NumericFeature::AttackMultiplier
    )].value
  );
  std::fprintf(
    file,
    "defense_multiplier=%u\n",
    settings.numeric_features[core::numeric_feature_index(
      core::NumericFeature::DefenseMultiplier
    )].value
  );
  std::fprintf(
    file,
    "movement_speed_multiplier_x10=%u\n",
    settings.numeric_features[core::numeric_feature_index(
      core::NumericFeature::MovementSpeedMultiplier
    )].value
  );
  std::fprintf(
    file,
    "zenny=%u\n",
    settings.numeric_features[core::numeric_feature_index(
      core::NumericFeature::Zenny
    )].value
  );
  std::fprintf(
    file,
    "wycademy_points=%u\n",
    settings.numeric_features[core::numeric_feature_index(
      core::NumericFeature::WycademyPoints
    )].value
  );
  std::fprintf(
    file,
    "item_pouch_slot=%u\n",
    static_cast<unsigned>(settings.item_pouch_slot)
  );
  std::fprintf(
    file,
    "item_pouch_quantity=%u\n",
    static_cast<unsigned>(settings.item_pouch_quantity)
  );
  const auto close_result = std::fclose(file);
  if (close_result != 0) {
    std::remove(temporary.c_str());
    return false;
  }
  const auto backup = path_ + ".bak";
  if (std::remove(backup.c_str()) != 0 && errno != ENOENT) {
    std::remove(temporary.c_str());
    return false;
  }
  const auto had_previous = std::rename(path_.c_str(), backup.c_str()) == 0;
  if (!had_previous && errno != ENOENT) {
    std::remove(temporary.c_str());
    return false;
  }
  if (std::rename(temporary.c_str(), path_.c_str()) != 0) {
    if (had_previous) {
      std::rename(backup.c_str(), path_.c_str());
    }
    std::remove(temporary.c_str());
    return false;
  }
  if (had_previous) {
    std::remove(backup.c_str());
  }
  return true;
}

}  // namespace mhgu::app
