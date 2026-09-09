#!/usr/bin/env python3
"""Compile normalized catalog and locale JSON into dependency-free C++ tables."""

from __future__ import annotations

import json
import unicodedata
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CATALOG_PATH = ROOT / "data" / "catalog" / "monsters.json"
FOOD_SKILL_CATALOG_PATH = ROOT / "data" / "catalog" / "food-skills.json"
LEGAL_RANGES_PATH = ROOT / "data" / "catalog" / "legal-size-ranges.json"
LOCALE_PATHS = {
  "en": ROOT / "data" / "locales" / "en.json",
  "zh-Hans": ROOT / "data" / "locales" / "zh-Hans.json",
  "ja": ROOT / "data" / "locales" / "ja.json",
}
CORE_OUTPUT = ROOT / "source" / "generated" / "catalog.cpp"
FOOD_SKILL_OUTPUT = ROOT / "source" / "generated" / "food_skills.cpp"
MESSAGES_OUTPUT = ROOT / "source" / "generated" / "messages.cpp"
ALIAS_OUTPUT = ROOT / "source" / "generated" / "switch_aliases.cpp"

UI_KEYS = [
  "title",
  "notRunning",
  "noMonsters",
  "size",
  "language",
  "automatic",
  "sizePreset",
  "off",
  "on",
  "mini",
  "silver",
  "gold",
  "scan",
  "scanning",
  "ready",
  "unsupported",
  "writeFailed",
  "hyper",
  "frameRate",
  "fps30",
  "fps60",
  "mapAndLargeMonsters",
  "carryItemsIntoPouch",
  "hunter",
  "invincible",
  "healthNoDecrease",
  "staminaNoDecrease",
  "sharpnessNoDecrease",
  "unlockHunterArtSlots",
  "unlimitedHunterArts",
  "valorGaugeNoDecrease",
  "alchemyGaugeFull",
  "spStatusNoExpire",
  "bowgunAutoReload",
  "consumableItemsNoDecrease",
  "transmog",
  "weaponTransmog",
  "armorTransmog",
  "palico",
  "palicoHealthNoDecrease",
  "palicoAffinity",
  "hunterAffinity",
  "spLevel",
  "longSwordSpiritGauge",
  "combatParameters",
  "monsterDamageMode",
  "instantKill",
  "leaveOneHp",
  "attackMultiplier",
  "defenseMultiplier",
  "movementSpeedMultiplier",
  "quest",
  "infiniteQuestTime",
  "unlimitedFaints",
  "completeQuest",
  "execute",
  "completed",
  "noActiveQuest",
  "failed",
  "resources",
  "zenny",
  "wycademyPoints",
  "itemPouchSlot",
  "itemPouchQuantity",
  "applyItemPouchQuantity",
  "monsterInfoOverlay",
  "hudLayout",
  "hudBottomLeftVertical",
  "hudTopRightVertical",
  "hudTopCenterHorizontal",
  "hudCenterLeftVertical",
  "hudCenterRightVertical",
  "hudContent",
  "hudMonsterInfo",
  "hudMonsterInfoAndDamage",
  "hudDamageOnly",
  "foodSkills",
  "foodSkill1",
  "foodSkill2",
  "foodSkill3",
  "applyFoodSkills",
  "noCharacterData",
  "damageDisplay",
  "hudDisplayMode",
  "damageOverlap",
  "damageSize",
  "damagePosition",
  "driftMode",
  "driftRandom",
  "driftUpward",
  "driftDistance",
  "driftSpeed",
  "staggerMode",
  "staggerVertical",
  "staggerHorizontal",
  "staggerUpward",
  "staggerDown",
  "staggerLeft",
  "staggerRight",
  "staggerType",
  "staggerZigzag",
  "staggerLinear",
  "resetDamageDisplay",
  "damageAppearEffect",
  "appearSizeAndColor",
  "appearSizeOnly",
  "appearColorOnly",
  "appearFixed",
]

TESLA_MENU_TEXT_KEYS = {
  "language",
  "automatic",
  "sizePreset",
  "off",
  "on",
  "mini",
  "silver",
  "gold",
  "scan",
  "frameRate",
  "fps30",
  "fps60",
  "mapAndLargeMonsters",
  "carryItemsIntoPouch",
  "hunter",
  "invincible",
  "healthNoDecrease",
  "staminaNoDecrease",
  "sharpnessNoDecrease",
  "unlockHunterArtSlots",
  "unlimitedHunterArts",
  "valorGaugeNoDecrease",
  "alchemyGaugeFull",
  "spStatusNoExpire",
  "bowgunAutoReload",
  "consumableItemsNoDecrease",
  "transmog",
  "weaponTransmog",
  "armorTransmog",
  "palico",
  "palicoHealthNoDecrease",
  "palicoAffinity",
  "hunterAffinity",
  "spLevel",
  "longSwordSpiritGauge",
  "combatParameters",
  "monsterDamageMode",
  "instantKill",
  "leaveOneHp",
  "attackMultiplier",
  "defenseMultiplier",
  "movementSpeedMultiplier",
  "quest",
  "infiniteQuestTime",
  "unlimitedFaints",
  "completeQuest",
  "execute",
  "completed",
  "noActiveQuest",
  "failed",
  "resources",
  "zenny",
  "wycademyPoints",
  "itemPouchSlot",
  "itemPouchQuantity",
  "applyItemPouchQuantity",
  "monsterInfoOverlay",
  "hudLayout",
  "hudBottomLeftVertical",
  "hudTopRightVertical",
  "hudTopCenterHorizontal",
  "hudCenterLeftVertical",
  "hudCenterRightVertical",
  "hudContent",
  "hudMonsterInfo",
  "hudMonsterInfoAndDamage",
  "hudDamageOnly",
  "foodSkills",
  "foodSkill1",
  "foodSkill2",
  "foodSkill3",
  "applyFoodSkills",
  "noCharacterData",
  "damageDisplay",
  "hudDisplayMode",
  "damageOverlap",
  "damageSize",
  "damagePosition",
  "driftMode",
  "driftRandom",
  "driftUpward",
  "driftDistance",
  "resetDamageDisplay",
}
# The overlay uses the full 1280px Tesla framebuffer for the game HUD and
# settings menu. Keep a guard against accidental runaway labels while allowing
# complete English descriptions instead of the old narrow-menu abbreviations.
TESLA_MENU_TEXT_MAX_CELLS = 32


def cpp_string(value: str) -> str:
  return json.dumps(value, ensure_ascii=False)


def display_cells(value: str) -> int:
  return sum(
    2 if unicodedata.east_asian_width(character) in {"F", "W"} else 1
    for character in value
  )


def validate_tesla_menu_text(locale_name: str, locale: dict) -> None:
  overlong = {
    key: display_cells(locale["ui"][key])
    for key in TESLA_MENU_TEXT_KEYS
    if display_cells(locale["ui"][key]) > TESLA_MENU_TEXT_MAX_CELLS
  }
  if overlong:
    details = ", ".join(f"{key}={width}" for key, width in sorted(overlong.items()))
    raise ValueError(
      f"{locale_name} Tesla menu text exceeds "
      f"{TESLA_MENU_TEXT_MAX_CELLS} display cells: {details}"
    )


def load_inputs() -> tuple[dict, dict, dict[str, dict], dict]:
  catalog = json.loads(CATALOG_PATH.read_text(encoding="utf-8"))
  food_skills = json.loads(
    FOOD_SKILL_CATALOG_PATH.read_text(encoding="utf-8")
  )
  legal_ranges = json.loads(LEGAL_RANGES_PATH.read_text(encoding="utf-8"))["monsters"]
  locales = {
    name: json.loads(path.read_text(encoding="utf-8"))
    for name, path in LOCALE_PATHS.items()
  }
  expected = {monster["key"] for monster in catalog["monsters"]}
  food_skill_rows = food_skills["skills"]
  expected_food_skill_ids = list(range(1, 66))
  actual_food_skill_ids = [skill["id"] for skill in food_skill_rows]
  if actual_food_skill_ids != expected_food_skill_ids:
    raise ValueError("food skill IDs must be contiguous from 1 through 65")
  expected_food_skills = {skill["key"] for skill in food_skill_rows}
  if len(expected_food_skills) != len(food_skill_rows):
    raise ValueError("food skill keys must be unique")
  for name, locale in locales.items():
    actual = set(locale["monsters"])
    if actual != expected:
      missing = sorted(expected - actual)
      extra = sorted(actual - expected)
      raise ValueError(f"{name} monster keys differ; missing={missing}, extra={extra}")
    actual_food_skills = set(locale["foodSkills"])
    if actual_food_skills != expected_food_skills:
      missing = sorted(expected_food_skills - actual_food_skills)
      extra = sorted(actual_food_skills - expected_food_skills)
      raise ValueError(
        f"{name} food skill keys differ; missing={missing}, extra={extra}"
      )
    missing_ui = [key for key in UI_KEYS if key not in locale["ui"]]
    if missing_ui:
      raise ValueError(f"{name} is missing UI keys: {missing_ui}")
    validate_tesla_menu_text(name, locale)

  expected_ranges = {
    monster["key"] for monster in catalog["monsters"] if monster["variableSize"]
  }
  actual_ranges = set(legal_ranges)
  if actual_ranges != expected_ranges:
    raise ValueError(
      "legal size range keys differ; "
      f"missing={sorted(expected_ranges - actual_ranges)}, "
      f"extra={sorted(actual_ranges - expected_ranges)}"
    )
  for monster in catalog["monsters"]:
    if not monster["variableSize"]:
      continue
    size_range = legal_ranges[monster["key"]]
    crowns = monster["crowns"]
    if (
      size_range["minPercent"] != crowns["miniPercent"]
      or size_range["maxPercent"] != crowns["goldPercent"]
      or not (
        50
        <= size_range["minPercent"]
        < 100
        < crowns["silverPercent"]
        <= size_range["maxPercent"]
        <= 200
      )
    ):
      raise ValueError(f"{monster['key']} has an invalid legal size range")
  return catalog, food_skills, locales, legal_ranges


def generate_core(
  catalog: dict,
  locales: dict[str, dict],
  legal_ranges: dict,
) -> str:
  rows = []
  for monster in catalog["monsters"]:
    key = monster["key"]
    crowns = monster["crowns"]
    size_range = legal_ranges.get(
      key,
      {"minPercent": 100, "maxPercent": 100},
    )
    rows.append(
      "  {"
      f"{monster['id']}, {cpp_string(key)}, "
      "{"
      f"{cpp_string(locales['en']['monsters'][key])}, "
      f"{cpp_string(locales['zh-Hans']['monsters'][key])}, "
      f"{cpp_string(locales['ja']['monsters'][key])}"
      "}, "
      f"{monster['baseSizeX100']}, "
      f"{crowns['miniPercent']}, "
      f"{crowns['silverPercent']}, "
      f"{crowns['goldPercent']}, "
      f"{size_range['minPercent']}, "
      f"{size_range['maxPercent']}, "
      f"{'true' if monster['variableSize'] else 'false'}"
      "},"
    )

  return f"""// Generated by tools/generate_catalog.py. Do not edit.
#include "mhgu/core/catalog.hpp"

#include <cstring>

namespace mhgu::core {{
namespace {{

constexpr MonsterDefinition kMonsters[] = {{
{chr(10).join(rows)}
}};

}}  // namespace

const MonsterDefinition* find_monster(const MonsterId id) {{
  for (const auto& monster : kMonsters) {{
    if (monster.id == id) {{
      return &monster;
    }}
  }}
  return nullptr;
}}

const MonsterDefinition* find_monster_by_key(const char* key) {{
  if (key == nullptr) {{
    return nullptr;
  }}
  for (const auto& monster : kMonsters) {{
    if (std::strcmp(monster.key, key) == 0) {{
      return &monster;
    }}
  }}
  return nullptr;
}}

const MonsterDefinition* monster_catalog() {{
  return kMonsters;
}}

std::size_t monster_catalog_size() {{
  return sizeof(kMonsters) / sizeof(kMonsters[0]);
}}

}}  // namespace mhgu::core
"""


def generate_messages(locales: dict[str, dict]) -> str:
  rows = []
  for key in UI_KEYS:
    rows.append(
      "  {"
      f"{cpp_string(locales['en']['ui'][key])}, "
      f"{cpp_string(locales['zh-Hans']['ui'][key])}, "
      f"{cpp_string(locales['ja']['ui'][key])}"
      "},"
    )
  return f"""// Generated by tools/generate_catalog.py. Do not edit.
#include "mhgu/core/messages.hpp"

#include <cstddef>

namespace mhgu::core {{
namespace {{

constexpr const char* kMessages[][3] = {{
{chr(10).join(rows)}
}};

}}  // namespace

const char* ui_message(const UiMessage message, const Locale locale) {{
  const auto row = static_cast<std::size_t>(message);
  auto column = std::size_t{{0}};
  if (locale == Locale::SimplifiedChinese) {{
    column = 1;
  }} else if (locale == Locale::Japanese) {{
    column = 2;
  }}
  return kMessages[row][column];
}}

}}  // namespace mhgu::core
"""


def generate_food_skills(
  food_skills: dict,
  locales: dict[str, dict],
) -> str:
  rows = []
  for skill in food_skills["skills"]:
    key = skill["key"]
    rows.append(
      "  {"
      f"{skill['id']}, {cpp_string(key)}, "
      "{"
      f"{cpp_string(locales['en']['foodSkills'][key])}, "
      f"{cpp_string(locales['zh-Hans']['foodSkills'][key])}, "
      f"{cpp_string(locales['ja']['foodSkills'][key])}"
      "}"
      "},"
    )

  return f"""// Generated by tools/generate_catalog.py. Do not edit.
#include "mhgu/core/food_skills.hpp"

namespace mhgu::core {{
namespace {{

constexpr FoodSkillDefinition kFoodSkills[] = {{
{chr(10).join(rows)}
}};

}}  // namespace

const FoodSkillDefinition* find_food_skill(const FoodSkillId id) {{
  if (id < kMinimumFoodSkillId || id > kMaximumFoodSkillId) {{
    return nullptr;
  }}
  const auto& skill = kFoodSkills[id - kMinimumFoodSkillId];
  return skill.id == id ? &skill : nullptr;
}}

const FoodSkillDefinition* food_skill_catalog() {{
  return kFoodSkills;
}}

std::size_t food_skill_catalog_size() {{
  return sizeof(kFoodSkills) / sizeof(kFoodSkills[0]);
}}

const char* food_skill_name(const FoodSkillId id, const Locale locale) {{
  const auto* skill = find_food_skill(id);
  if (skill == nullptr) {{
    return "";
  }}
  switch (locale) {{
    case Locale::SimplifiedChinese:
      return skill->names.simplified_chinese;
    case Locale::Japanese:
      return skill->names.japanese;
    case Locale::English:
    default:
      return skill->names.english;
  }}
}}

}}  // namespace mhgu::core
"""


def generate_aliases(catalog: dict) -> str:
  rows = [
    (
      int(monster["switch"]["rawId"], 16),
      monster["id"],
    )
    for monster in catalog["monsters"]
  ]
  rows.sort()
  entries = [f"  {{0x{raw_id:06X}, {monster_id}}}," for raw_id, monster_id in rows]
  return f"""// Generated by tools/generate_catalog.py. Do not edit.
#include "mhgu/platform/switch/monster_aliases.hpp"

namespace mhgu::platform::switch_adapter {{
namespace {{

constexpr MonsterAlias kAliases[] = {{
{chr(10).join(entries)}
}};

}}  // namespace

const MonsterAlias* find_monster_alias(const std::uint32_t raw_id) {{
  for (const auto& alias : kAliases) {{
    if (alias.raw_id == raw_id) {{
      return &alias;
    }}
  }}
  return nullptr;
}}

const MonsterAlias* monster_aliases() {{
  return kAliases;
}}

std::size_t monster_alias_count() {{
  return sizeof(kAliases) / sizeof(kAliases[0]);
}}

}}  // namespace mhgu::platform::switch_adapter
"""


def main() -> None:
  catalog, food_skills, locales, legal_ranges = load_inputs()
  outputs = {
    CORE_OUTPUT: generate_core(catalog, locales, legal_ranges),
    FOOD_SKILL_OUTPUT: generate_food_skills(food_skills, locales),
    MESSAGES_OUTPUT: generate_messages(locales),
    ALIAS_OUTPUT: generate_aliases(catalog),
  }
  for path, content in outputs.items():
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")
  print(
    f"Generated C++ tables for {len(catalog['monsters'])} monsters and "
    f"{len(food_skills['skills'])} food skills"
  )


if __name__ == "__main__":
  main()
