#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "mhgu/core/catalog.hpp"
#include "mhgu/core/food_skills.hpp"
#include "mhgu/core/locale.hpp"

int main() {
  using namespace mhgu::core;

  assert(monster_catalog_size() == 94);
  assert(food_skill_catalog_size() == 65);

  const auto* food_skills = food_skill_catalog();
  for (std::size_t index = 0; index < food_skill_catalog_size(); ++index) {
    const auto& skill = food_skills[index];
    assert(skill.id == index + 1);
    assert(skill.key != nullptr && skill.key[0] != '\0');
    assert(food_skill_name(skill.id, Locale::English)[0] != '\0');
    assert(food_skill_name(skill.id, Locale::SimplifiedChinese)[0] != '\0');
    assert(food_skill_name(skill.id, Locale::Japanese)[0] != '\0');
  }
  assert(find_food_skill(0) == nullptr);
  assert(find_food_skill(66) == nullptr);
  assert(
    std::strcmp(food_skill_name(11, Locale::English), "Felyne Charisma") == 0
  );
  assert(
    std::strcmp(food_skill_name(11, Locale::Japanese), "ネコのカリスマ") == 0
  );
  assert(
    std::strcmp(
      food_skill_name(18, Locale::SimplifiedChinese), "猫的火场怪力"
    ) == 0
  );
  assert(
    std::strcmp(
      food_skill_name(40, Locale::SimplifiedChinese), "猫的击晕术"
    ) == 0
  );
  assert(
    std::strcmp(
      food_skill_name(64, Locale::Japanese), "ネコのあとは任せた！"
    ) == 0
  );

  const auto* catalog = monster_catalog();
  for (std::size_t index = 0; index < monster_catalog_size(); ++index) {
    const auto& monster = catalog[index];
    assert(monster.id == index + 1);
    assert(monster.key != nullptr && monster.key[0] != '\0');
    assert(localized_name(monster, Locale::English)[0] != '\0');
    assert(localized_name(monster, Locale::SimplifiedChinese)[0] != '\0');
    assert(localized_name(monster, Locale::Japanese)[0] != '\0');

    if (monster.variable_size) {
      assert(monster.base_size_x100 > 0);
      assert(monster.mini_percent > 0);
      assert(monster.mini_percent < 100);
      assert(monster.silver_percent > 100);
      assert(monster.gold_percent >= monster.silver_percent);
      assert(monster.legal_min_percent == monster.mini_percent);
      assert(monster.legal_max_percent == monster.gold_percent);
      assert(monster.legal_min_percent < 100);
      assert(monster.legal_max_percent <= 200);
    } else {
      assert(monster.legal_min_percent == 100);
      assert(monster.legal_max_percent == 100);
    }
  }

  const auto* rathian = find_monster_by_key("rathian");
  assert(rathian != nullptr);
  assert(rathian->base_size_x100 == 165430);
  assert(rathian->mini_percent == 90);
  assert(rathian->silver_percent == 115);
  assert(rathian->gold_percent == 123);
  assert(rathian->legal_min_percent == 90);
  assert(rathian->legal_max_percent == 123);
  assert(std::strcmp(rathian->names.english, "Rathian") == 0);
  assert(std::strcmp(rathian->names.simplified_chinese, "雌火龙") == 0);
  assert(std::strcmp(rathian->names.japanese, "リオレイア") == 0);

  std::cout << "catalog tests passed\n";
}
