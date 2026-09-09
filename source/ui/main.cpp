#define TESLA_INIT_IMPL
#include <tesla.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "mhgu/app/model.hpp"
#include "mhgu/core/food_skills.hpp"
#include "mhgu/core/locale.hpp"
#include "mhgu/core/messages.hpp"

namespace {

using mhgu::app::FoodSkillApplyStatus;
using mhgu::app::Model;
using mhgu::app::QuestCompletionStatus;
using mhgu::core::DamageAppearEffect;
using mhgu::core::DamageDriftMode;
using mhgu::core::DamageStaggerMode;
using mhgu::core::DamageStaggerType;
using mhgu::core::HudContent;
using mhgu::core::HudLayout;
using mhgu::core::Locale;
using mhgu::core::LocaleMode;
using mhgu::core::MonsterDamageMode;
using mhgu::core::NumericFeature;
using mhgu::core::RuntimeFeature;
using mhgu::core::SizePreset;
using mhgu::core::UiMessage;
using mhgu::platform::switch_adapter::SessionStatus;

#ifndef MHGU_OVERLAY_VERSION
#define MHGU_OVERLAY_VERSION "development"
#endif

constexpr const char* kVersion = "v" MHGU_OVERLAY_VERSION;

std::uint64_t monotonic_milliseconds() {
  return static_cast<std::uint64_t>(
    std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()
    ).count()
  );
}

// Monotonic timestamp of the last time the Tesla overlay became visible.
std::uint64_t g_overlay_shown_at_ms = 0;

// Handles L + Down on any menu page: hides the overlay and returns to the
// game without popping the Gui stack.  The stack keeps the current page
// alive, so reopening the Tesla menu lands on the exact page the player
// left.  The chord that opens the menu (L + Down + R3 by default) is still
// queued as freshly pressed keys on the first visible frame, so the combo
// is ignored briefly after every show to avoid immediately hiding again.
inline bool handle_minimize_combo(
  const u64 keys_down,
  const u64 keys_held
) {
  constexpr std::uint64_t kReopenGuardMs = 300;
  if (monotonic_milliseconds() - g_overlay_shown_at_ms < kReopenGuardMs) {
    return false;
  }
  if ((keys_down & HidNpadButton_Down) != 0 &&
      (keys_held & HidNpadButton_L) != 0) {
    tsl::Overlay::get()->hide();
    return true;
  }
  return false;
}

#include "components/damage_text.hpp"
#include "components/menu_items.hpp"
#include "components/hud.hpp"
#include "components/submenus.hpp"
#include "components/main_menu.hpp"

class MhguOverlay final : public tsl::Overlay {
public:
  void initServices() override {
    model_.start();
  }

  void exitServices() override {
    model_.stop();
  }

  void onShow() override {
    g_overlay_shown_at_ms = monotonic_milliseconds();
  }

  std::unique_ptr<tsl::Gui> loadInitialGui() override {
    return initially<MainGui>(model_);
  }

private:
  Model model_;
};

}  // namespace

int main(const int argc, char** argv) {
  // libtesla defaults to a narrow 448px framebuffer for its settings UI.
  // The hunting HUD has left/right layout presets, so render it in the full
  // 1280px logical screen width before the overlay allocates its framebuffer.
  framebufferWidth = 1280;
  framebufferHeight = 720;
  return tsl::loop<MhguOverlay>(argc, argv);
}
