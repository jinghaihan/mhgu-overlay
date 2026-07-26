#pragma once

#include <cstdint>

#include "mhgu/core/types.hpp"

namespace mhgu::core {

enum class UiMessage : std::uint8_t {
    Title,
    NotRunning,
    NoMonsters,
    Health,
    Size,
    Language,
    Automatic,
    SizePreset,
    Off,
    Mini,
    Silver,
    Gold,
    Experimental,
    Scan,
    Scanning,
    Ready,
    Unsupported,
    WriteFailed,
    Hyper,
    Hud,
    BackHint,
};

const char* ui_message(UiMessage message, Locale locale);

}  // namespace mhgu::core
