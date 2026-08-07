#pragma once

#include <cstdint>

#include "mhgu/core/types.hpp"

namespace mhgu::core {

enum class UiMessage : std::uint8_t {
  Title,
  NotRunning,
  NoMonsters,
  Size,
  Language,
  Automatic,
  SizePreset,
  Off,
  Mini,
  Silver,
  Gold,
  Scan,
  Scanning,
  Ready,
  Unsupported,
  WriteFailed,
  Hyper,
  FrameRate,
  Fps30,
  Fps60,
  OpenOverlay,
};

const char* ui_message(UiMessage message, Locale locale);

}  // namespace mhgu::core
