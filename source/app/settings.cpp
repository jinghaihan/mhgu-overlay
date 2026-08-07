#include "mhgu/app/settings.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>

#ifdef __SWITCH__
#include <sys/stat.h>
#endif

namespace mhgu::app {
namespace {

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
    return 7777777;
  }
  if (parsed < 0) {
    return 0;
  }
  if (parsed > 9999999) {
    return 9999999;
  }
  return static_cast<std::uint32_t>(parsed);
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

}  // namespace

SettingsStore::SettingsStore(std::string path)
  : path_(std::move(path)) {}

core::CoreSettings SettingsStore::load() const {
  core::CoreSettings settings{};
  bool has_legacy_size_lock = false;
  bool legacy_size_lock_enabled = false;
  auto* file = std::fopen(path_.c_str(), "r");
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
    } else if (std::strcmp(key, "size_lock") == 0) {
      has_legacy_size_lock = true;
      legacy_size_lock_enabled = std::strcmp(value, "1") == 0;
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
  const auto close_result = std::fclose(file);
  if (close_result != 0) {
    std::remove(temporary.c_str());
    return false;
  }
  if (std::rename(temporary.c_str(), path_.c_str()) != 0) {
    std::remove(temporary.c_str());
    return false;
  }
  return true;
}

}  // namespace mhgu::app
