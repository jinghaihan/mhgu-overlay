#include "mhgu/core/size.hpp"

namespace mhgu::core {

std::uint16_t size_percent_for_preset(
    const MonsterDefinition& monster,
    const SizePreset preset
) {
    if (!monster.variable_size) {
        return 100;
    }

    switch (preset) {
        case SizePreset::Mini:
            return monster.mini_percent;
        case SizePreset::Silver:
            return monster.silver_percent;
        case SizePreset::Gold:
            return monster.gold_percent;
        case SizePreset::Off:
        default:
            return 0;
    }
}

std::uint32_t actual_size_x100(
    const MonsterDefinition& monster,
    const std::uint16_t size_percent
) {
    return static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(monster.base_size_x100) * size_percent + 50U) /
        100U
    );
}

Crown classify_crown(
    const MonsterDefinition& monster,
    const std::uint16_t size_percent
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
