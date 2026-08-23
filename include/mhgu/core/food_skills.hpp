#pragma once

#include "mhgu/core/types.hpp"

namespace mhgu::core {

const FoodSkillDefinition* find_food_skill(FoodSkillId id);
const FoodSkillDefinition* food_skill_catalog();
std::size_t food_skill_catalog_size();
const char* food_skill_name(FoodSkillId id, Locale locale);

}  // namespace mhgu::core
