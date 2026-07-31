#include "mhgu/core/engine.hpp"

#include <algorithm>

#include "mhgu/core/catalog.hpp"
#include "mhgu/core/locale.hpp"
#include "mhgu/core/size.hpp"

namespace mhgu::core {

CoreOutput Engine::update(
  const GameSnapshot& snapshot, const CoreSettings& settings
) const {
  CoreOutput output{};
  output.locale = resolve_locale(
    snapshot.game, settings.locale_mode, snapshot.detected_locale
  );

  const auto count = std::min(snapshot.monster_count, kMaxMonsters);
  for (std::size_t index = 0; index < count; ++index) {
    const auto& source = snapshot.monsters[index];
    const auto* definition = find_monster(source.monster_id);
    if (definition == nullptr || source.handle == 0 || source.max_hp == 0) {
      continue;
    }

    auto& view = output.monsters[output.monster_count++];
    view.handle = source.handle;
    view.monster_id = source.monster_id;
    view.name = localized_name(*definition, output.locale);
    view.hp = source.hp;
    view.max_hp = source.max_hp;
    view.hp_percent_x10 = static_cast<std::uint16_t>(std::min<std::uint64_t>(
      1000U, (static_cast<std::uint64_t>(source.hp) * 1000U) / source.max_hp
    ));
    view.size_percent = source.size_percent;
    view.actual_size_x100 = actual_size_x100(*definition, source.size_percent);
    view.crown = classify_crown(*definition, source.size_percent);
    view.hyper = source.hyper;

    if (source.hp == 0 || settings.size_preset == SizePreset::Off ||
        !definition->variable_size) {
      continue;
    }

    const auto target =
      size_percent_for_preset(*definition, settings.size_preset);
    if (target != 0 && target != source.size_percent) {
      output.writes[output.write_count++] = {
        source.handle,
        source.monster_id,
        target,
      };
    }
  }

  return output;
}

}  // namespace mhgu::core
