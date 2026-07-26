#pragma once

#include <cstddef>
#include <cstdint>

#include "mhgu/core/types.hpp"

namespace mhgu::platform::switch_adapter {

struct MonsterAlias {
    std::uint32_t raw_id;
    core::MonsterId monster_id;
};

const MonsterAlias* find_monster_alias(std::uint32_t raw_id);
const MonsterAlias* monster_aliases();
std::size_t monster_alias_count();

}  // namespace mhgu::platform::switch_adapter
