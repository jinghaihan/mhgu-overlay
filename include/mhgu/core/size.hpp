#pragma once

#include "mhgu/core/types.hpp"

namespace mhgu::core {

std::uint16_t size_percent_for_preset(
    const MonsterDefinition& monster,
    SizePreset preset
);

std::uint32_t actual_size_x100(
    const MonsterDefinition& monster,
    std::uint16_t size_percent
);

Crown classify_crown(
    const MonsterDefinition& monster,
    std::uint16_t size_percent
);

}  // namespace mhgu::core
