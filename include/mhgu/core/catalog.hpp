#pragma once

#include "mhgu/core/types.hpp"

namespace mhgu::core {

const MonsterDefinition* find_monster(MonsterId id);
const MonsterDefinition* find_monster_by_key(const char* key);
const MonsterDefinition* monster_catalog();
std::size_t monster_catalog_size();

}  // namespace mhgu::core
