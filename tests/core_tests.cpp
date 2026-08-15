#include <cassert>
#include <cstring>
#include <iostream>

#include "mhgu/core/engine.hpp"
#include "mhgu/core/locale.hpp"
#include "mhgu/core/messages.hpp"
#include "mhgu/core/size.hpp"

namespace mhgu::core {

namespace {

constexpr MonsterDefinition kRathian{
  1,
  "rathian",
  {"Rathian", "雌火龙", "リオレイア"},
  165430,
  90,
  115,
  123,
  90,
  123,
  true,
};

}  // namespace

const MonsterDefinition* find_monster(const MonsterId id) {
  return id == kRathian.id ? &kRathian : nullptr;
}

const MonsterDefinition* find_monster_by_key(const char* key) {
  return key != nullptr && std::strcmp(key, "rathian") == 0 ? &kRathian
                                                            : nullptr;
}

const MonsterDefinition* monster_catalog() {
  return &kRathian;
}

std::size_t monster_catalog_size() {
  return 1;
}

}  // namespace mhgu::core

int main() {
  using namespace mhgu::core;

  const auto* rathian = find_monster_by_key("rathian");
  assert(rathian != nullptr);
  assert(monster_catalog_size() == 1);
  assert(size_percent_for_preset(*rathian, SizePreset::Mini) == 90);
  assert(size_percent_for_preset(*rathian, SizePreset::Silver) == 115);
  assert(size_percent_for_preset(*rathian, SizePreset::Gold) == 123);
  assert(is_legal_size_percent(*rathian, 90));
  assert(is_legal_size_percent(*rathian, 115));
  assert(is_legal_size_percent(*rathian, 123));
  assert(!is_legal_size_percent(*rathian, 89));
  assert(!is_legal_size_percent(*rathian, 124));
  auto invalid_range = *rathian;
  invalid_range.legal_max_percent = 122;
  assert(size_percent_for_preset(invalid_range, SizePreset::Gold) == 0);
  assert(actual_size_x100(*rathian, 123) == 203479);
  assert(classify_crown(*rathian, 90) == Crown::Mini);
  assert(classify_crown(*rathian, 115) == Crown::Silver);
  assert(classify_crown(*rathian, 123) == Crown::Gold);
  assert(classify_crown(*rathian, 100) == Crown::None);

  assert(
    resolve_locale(GameId::Mhxx, LocaleMode::Auto, Locale::English) ==
    Locale::Japanese
  );
  assert(
    resolve_locale(GameId::Mhgu, LocaleMode::Auto, Locale::SimplifiedChinese) ==
    Locale::SimplifiedChinese
  );
  assert(
    std::strcmp(localized_name(*rathian, Locale::Japanese), "リオレイア") == 0
  );
  assert(
    std::strcmp(
      ui_message(UiMessage::Scanning, Locale::English),
      "Waiting for monster data"
    ) == 0
  );
  assert(
    std::strcmp(
      ui_message(UiMessage::Scanning, Locale::SimplifiedChinese),
      "等待怪物数据"
    ) == 0
  );
  assert(
    std::strcmp(
      ui_message(UiMessage::Scanning, Locale::Japanese),
      "モンスターデータ待機中"
    ) == 0
  );
  assert(
    std::strcmp(
      ui_message(UiMessage::HudDamageOnly, Locale::English),
      "Damage only"
    ) == 0
  );
  assert(
    std::strcmp(
      ui_message(UiMessage::HudDamageOnly, Locale::SimplifiedChinese),
      "仅伤害数字"
    ) == 0
  );
  assert(
    std::strcmp(
      ui_message(UiMessage::HudDamageOnly, Locale::Japanese),
      "ダメージのみ"
    ) == 0
  );

  GameSnapshot snapshot{};
  snapshot.game = GameId::Mhgu;
  snapshot.detected_locale = Locale::SimplifiedChinese;
  snapshot.monster_count = 1;
  snapshot.monsters[0] = {
    0x1234,
    rathian->id,
    4000,
    5000,
    100,
    false,
  };

  CoreSettings settings{};
  settings.locale_mode = LocaleMode::Auto;
  settings.size_preset = SizePreset::Gold;

  const Engine engine;
  const auto output = engine.update(snapshot, settings);
  assert(output.locale == Locale::SimplifiedChinese);
  assert(output.monster_count == 1);
  assert(output.monsters[0].hp_percent_x10 == 800);
  assert(output.write_count == 1);
  assert(output.writes[0].target_percent == 123);

  snapshot.monsters[0].hp = 0;
  const auto defeated = engine.update(snapshot, settings);
  assert(defeated.write_count == 0);

  snapshot.monsters[0].hp = 4000;
  snapshot.monsters[0].size_percent = 123;
  const auto verified = engine.update(snapshot, settings);
  assert(verified.write_count == 0);
  assert(verified.monsters[0].crown == Crown::Gold);

  std::cout << "core tests passed\n";
}
