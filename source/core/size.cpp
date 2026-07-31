#include "mhgu/core/size.hpp"

namespace mhgu::core {

bool is_legal_size_percent(
  const MonsterDefinition& monster, const std::uint16_t size_percent
) {
  if (!monster.variable_size) {
    return size_percent == 100;
  }
  return monster.legal_min_percent > 0 &&
         monster.legal_min_percent <= monster.legal_max_percent &&
         size_percent >= monster.legal_min_percent &&
         size_percent <= monster.legal_max_percent;
}

std::uint16_t size_percent_for_preset(
  const MonsterDefinition& monster, const SizePreset preset
) {
  if (!monster.variable_size) {
    return preset == SizePreset::Off ? 0 : 100;
  }

  std::uint16_t target{};
  switch (preset) {
    case SizePreset::Mini:
      target = monster.mini_percent;
      break;
    case SizePreset::Silver:
      target = monster.silver_percent;
      break;
    case SizePreset::Gold:
      target = monster.gold_percent;
      break;
    case SizePreset::Off:
    default:
      return 0;
  }
  return is_legal_size_percent(monster, target) ? target : 0;
}

std::uint32_t actual_size_x100(
  const MonsterDefinition& monster, const std::uint16_t size_percent
) {
  return static_cast<std::uint32_t>(
    (static_cast<std::uint64_t>(monster.base_size_x100) * size_percent + 50U) /
    100U
  );
}

Crown classify_crown(
  const MonsterDefinition& monster, const std::uint16_t size_percent
) {
  if (!monster.variable_size) {
    return Crown::None;
  }
  if (size_percent >= monster.gold_percent) {
    return Crown::Gold;
  }
  if (size_percent >= monster.silver_percent) {
    return Crown::Silver;
  }
  if (size_percent <= monster.mini_percent) {
    return Crown::Mini;
  }
  return Crown::None;
}

}  // namespace mhgu::core
