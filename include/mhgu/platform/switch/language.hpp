#pragma once

#include <cstdint>

#include "mhgu/core/types.hpp"

namespace mhgu::platform::switch_adapter {

core::Locale locale_from_switch_language(std::int32_t language);
core::Locale detect_game_locale(std::uint64_t title_id);

}  // namespace mhgu::platform::switch_adapter
