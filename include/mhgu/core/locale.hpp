#pragma once

#include "mhgu/core/types.hpp"

namespace mhgu::core {

Locale resolve_locale(GameId game, LocaleMode mode, Locale detected_locale);

const char* localized_name(const MonsterDefinition& monster, Locale locale);

const char* crown_label(Crown crown, Locale locale);
const char* hyper_label(Locale locale);
const char* size_preset_label(SizePreset preset, Locale locale);

}  // namespace mhgu::core
