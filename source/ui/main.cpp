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
#include "mhgu/core/locale.hpp"
#include "mhgu/core/messages.hpp"

namespace {

using mhgu::app::Model;
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

const char* text(Model& model, const UiMessage message) {
  return mhgu::core::ui_message(message, model.display_locale());
}

const char* language_value(Model& model) {
  switch (model.settings().locale_mode) {
    case LocaleMode::English:
      return "English";
    case LocaleMode::SimplifiedChinese:
      return "简体中文";
    case LocaleMode::Japanese:
      return "日本語";
    default:
      return text(model, UiMessage::Automatic);
  }
}

const char* frame_rate_value(Model& model) {
  return text(
    model,
    model.settings().frame_rate == mhgu::core::FrameRate::Fps60
      ? UiMessage::Fps60
      : UiMessage::Fps30
  );
}

bool runtime_feature_enabled(
  Model& model, const RuntimeFeature feature
) {
  const auto settings = model.settings();
  const auto index = mhgu::core::runtime_feature_index(feature);
  return index < settings.runtime_features.size() &&
         settings.runtime_features[index];
}

void refresh_runtime_feature_item(
  tsl::elm::ListItem* item,
  Model& model,
  const UiMessage label,
  const RuntimeFeature feature
) {
  const auto locale = model.display_locale();
  item->setText(mhgu::core::ui_message(label, locale));
  item->setValue(mhgu::core::ui_message(
    runtime_feature_enabled(model, feature) ? UiMessage::On : UiMessage::Off,
    locale
  ));
}

tsl::elm::ListItem* runtime_feature_item(
  Model& model,
  const UiMessage label,
  const RuntimeFeature feature
) {
  auto* item = new tsl::elm::ListItem(
    mhgu::core::ui_message(label, model.display_locale())
  );
  refresh_runtime_feature_item(item, model, label, feature);
  item->setClickListener(
    [model_ptr = &model, item, feature](const u64 keys) {
      if ((keys & HidNpadButton_A) == 0 ||
          runtime_feature_enabled(*model_ptr, feature)) {
        return false;
      }
      model_ptr->enable_runtime_feature(feature);
      item->setValue(mhgu::core::ui_message(
        UiMessage::On, model_ptr->display_locale()
      ));
      return true;
    }
  );
  return item;
}

const char* monster_damage_mode_value(Model& model) {
  switch (model.settings().monster_damage_mode) {
    case MonsterDamageMode::InstantKill:
      return text(model, UiMessage::InstantKill);
    case MonsterDamageMode::LeaveOneHp:
      return text(model, UiMessage::LeaveOneHp);
    default:
      return text(model, UiMessage::Off);
  }
}

tsl::elm::ListItem* monster_damage_mode_item(Model& model) {
  auto* item = new tsl::elm::ListItem(
    text(model, UiMessage::MonsterDamageMode)
  );
  item->setValue(monster_damage_mode_value(model));
  item->setClickListener(
    [model_ptr = &model, item](const u64 keys) {
      int direction{};
      if ((keys & HidNpadButton_Left) != 0) {
        direction = -1;
      } else if ((keys & (HidNpadButton_A | HidNpadButton_Right)) != 0) {
        direction = 1;
      } else {
        return false;
      }
      model_ptr->cycle_monster_damage_mode(direction);
      item->setText(text(*model_ptr, UiMessage::MonsterDamageMode));
      item->setValue(monster_damage_mode_value(*model_ptr));
      return true;
    }
  );
  return item;
}

void refresh_item_pouch_slot_item(tsl::elm::ListItem* item, Model& model) {
  char value[8]{};
  std::snprintf(
    value,
    sizeof(value),
    "%u",
    static_cast<unsigned>(model.settings().item_pouch_slot)
  );
  item->setText(text(model, UiMessage::ItemPouchSlot));
  item->setValue(value);
}

tsl::elm::ListItem* item_pouch_slot_item(Model& model) {
  auto* item = new tsl::elm::ListItem(
    text(model, UiMessage::ItemPouchSlot)
  );
  refresh_item_pouch_slot_item(item, model);
  item->setClickListener(
    [model_ptr = &model, item](const u64 keys) {
      int delta{};
      if ((keys & HidNpadButton_Left) != 0) {
        delta = -1;
      } else if ((keys & HidNpadButton_Right) != 0) {
        delta = 1;
      } else if ((keys & HidNpadButton_L) != 0) {
        delta = -5;
      } else if ((keys & HidNpadButton_R) != 0) {
        delta = 5;
      } else {
        return false;
      }
      model_ptr->adjust_item_pouch_slot(delta);
      refresh_item_pouch_slot_item(item, *model_ptr);
      return true;
    }
  );
  return item;
}

void refresh_item_pouch_quantity_item(
  tsl::elm::ListItem* item, Model& model
) {
  char value[8]{};
  std::snprintf(
    value,
    sizeof(value),
    "%u",
    static_cast<unsigned>(model.settings().item_pouch_quantity)
  );
  item->setText(text(model, UiMessage::ItemPouchQuantity));
  item->setValue(value);
}

tsl::elm::ListItem* item_pouch_quantity_item(Model& model) {
  auto* item = new tsl::elm::ListItem(
    text(model, UiMessage::ItemPouchQuantity)
  );
  refresh_item_pouch_quantity_item(item, model);
  item->setClickListener(
    [model_ptr = &model, item](const u64 keys) {
      int delta{};
      if ((keys & HidNpadButton_Left) != 0) {
        delta = -1;
      } else if ((keys & HidNpadButton_Right) != 0) {
        delta = 1;
      } else if ((keys & HidNpadButton_L) != 0) {
        delta = -10;
      } else if ((keys & HidNpadButton_R) != 0) {
        delta = 10;
      } else {
        return false;
      }
      model_ptr->adjust_item_pouch_quantity(delta);
      refresh_item_pouch_quantity_item(item, *model_ptr);
      return true;
    }
  );
  return item;
}

tsl::elm::ListItem* apply_item_pouch_quantity_item(Model& model) {
  auto* item = new tsl::elm::ListItem(
    text(model, UiMessage::ApplyItemPouchQuantity)
  );
  item->setClickListener(
    [model_ptr = &model](const u64 keys) {
      if ((keys & HidNpadButton_A) == 0) {
        return false;
      }
      model_ptr->request_item_pouch_quantity_write();
      return true;
    }
  );
  return item;
}

mhgu::core::NumericFeatureSetting numeric_feature_setting(
  Model& model, const NumericFeature feature
) {
  const auto settings = model.settings();
  const auto index = mhgu::core::numeric_feature_index(feature);
  if (index >= settings.numeric_features.size()) {
    return {};
  }
  return settings.numeric_features[index];
}

void refresh_numeric_feature_item(
  tsl::elm::ListItem* item,
  Model& model,
  const UiMessage label,
  const NumericFeature feature
) {
  const auto locale = model.display_locale();
  const auto setting = numeric_feature_setting(model, feature);
  char value[32]{};
  const auto state = mhgu::core::ui_message(
    setting.enabled ? UiMessage::On : UiMessage::Off, locale
  );
  if (feature == NumericFeature::SpLevel) {
    std::snprintf(value, sizeof(value), "%s / Lv. %u", state, setting.value);
  } else if (feature == NumericFeature::AttackMultiplier ||
             feature == NumericFeature::DefenseMultiplier) {
    std::snprintf(value, sizeof(value), "%s / x%u", state, setting.value);
  } else if (feature == NumericFeature::MovementSpeedMultiplier) {
    std::snprintf(
      value,
      sizeof(value),
      "%s / x%u.%u",
      state,
      setting.value / 10,
      setting.value % 10
    );
  } else if (feature == NumericFeature::Zenny ||
             feature == NumericFeature::WycademyPoints) {
    std::snprintf(value, sizeof(value), "%s / %u", state, setting.value);
  } else {
    std::snprintf(value, sizeof(value), "%s / %u%%", state, setting.value);
  }
  item->setText(mhgu::core::ui_message(label, locale));
  item->setValue(value);
}

int numeric_feature_small_step(const NumericFeature feature) {
  return feature == NumericFeature::Zenny ||
             feature == NumericFeature::WycademyPoints
           ? 10000
           : 1;
}

int numeric_feature_large_step(const NumericFeature feature) {
  if (feature == NumericFeature::Zenny ||
      feature == NumericFeature::WycademyPoints) {
    return 1000000;
  }
  if (feature == NumericFeature::MovementSpeedMultiplier) {
    return 5;
  }
  return feature == NumericFeature::SpLevel ||
             feature == NumericFeature::AttackMultiplier ||
             feature == NumericFeature::DefenseMultiplier
           ? 1
           : 10;
}

tsl::elm::ListItem* numeric_feature_item(
  Model& model,
  const UiMessage label,
  const NumericFeature feature
) {
  auto* item = new tsl::elm::ListItem(
    mhgu::core::ui_message(label, model.display_locale())
  );
  refresh_numeric_feature_item(item, model, label, feature);
  item->setClickListener(
    [model_ptr = &model, item, label, feature](const u64 keys) {
      int delta{};
      if ((keys & HidNpadButton_Left) != 0) {
        delta = -numeric_feature_small_step(feature);
      } else if ((keys & HidNpadButton_Right) != 0) {
        delta = numeric_feature_small_step(feature);
      } else if ((keys & HidNpadButton_L) != 0) {
        delta = -numeric_feature_large_step(feature);
      } else if ((keys & HidNpadButton_R) != 0) {
        delta = numeric_feature_large_step(feature);
      } else if ((keys & HidNpadButton_A) != 0) {
        model_ptr->enable_numeric_feature(feature);
        refresh_numeric_feature_item(
          item, *model_ptr, label, feature
        );
        return true;
      } else {
        return false;
      }
      model_ptr->adjust_numeric_feature(feature, delta);
      refresh_numeric_feature_item(item, *model_ptr, label, feature);
      return true;
    }
  );
  return item;
}

const char* status_value(const SessionStatus status, const Locale locale) {
  switch (status) {
    case SessionStatus::Unsupported:
      return mhgu::core::ui_message(UiMessage::Unsupported, locale);
    case SessionStatus::Searching:
      return mhgu::core::ui_message(UiMessage::Scanning, locale);
    case SessionStatus::Ready:
      return mhgu::core::ui_message(UiMessage::Ready, locale);
    case SessionStatus::WriteFailed:
      return mhgu::core::ui_message(UiMessage::WriteFailed, locale);
    case SessionStatus::ReadFailed:
      return mhgu::core::ui_message(UiMessage::Scan, locale);
    default:
      return mhgu::core::ui_message(UiMessage::NotRunning, locale);
  }
}

tsl::gfx::Color crown_color(const mhgu::core::Crown crown) {
  switch (crown) {
    case mhgu::core::Crown::Mini:
      return {0x4, 0xB, 0xF, 0xF};
    case mhgu::core::Crown::Silver:
      return {0xC, 0xD, 0xE, 0xF};
    case mhgu::core::Crown::Gold:
      return {0xF, 0xC, 0x3, 0xF};
    default:
      return {0xF, 0xF, 0xF, 0xF};
  }
}

class LocalizedOverlayFrame final : public tsl::elm::OverlayFrame {
public:
  using tsl::elm::OverlayFrame::OverlayFrame;

  void setTitle(std::string title) {
    m_title = std::move(title);
  }
};

class HudElement final : public tsl::elm::Element {
public:
  explicit HudElement(Model& model)
    : model_(model) {}

  void draw(tsl::gfx::Renderer* renderer) override {
    renderer->clearScreen();
    const auto view = model_.session_view();
    const auto settings = model_.settings();
    const auto locale = model_.display_locale();
    const auto count = std::min<std::size_t>(
      view.output.monster_count, mhgu::core::kMaxMonsters
    );

    if (count == 0) {
      const auto* message =
        view.status == SessionStatus::Ready
          ? mhgu::core::ui_message(UiMessage::NoMonsters, locale)
          : status_value(view.status, locale);
      draw_status(renderer, locale, message);
    } else {
      const auto stack_height =
        static_cast<s32>(count * kCardHeight + (count - 1) * kCardGap);
      const s32 first_y =
        tsl::cfg::FramebufferHeight - kMargin - stack_height;
      for (std::size_t index = 0; index < count; ++index) {
        draw_monster(
          renderer,
          view.output.monsters[index],
          locale,
          kMargin,
          first_y + static_cast<s32>(index * (kCardHeight + kCardGap))
        );
      }
    }
    if (settings.damage_display_enabled) {
      draw_damage_events(renderer, view.damage, monotonic_milliseconds());
    }
  }

  void layout(
    const u16 parent_x,
    const u16 parent_y,
    const u16 parent_width,
    const u16 parent_height
  ) override {
    setBoundaries(parent_x, parent_y, parent_width, parent_height);
  }

  tsl::elm::Element*
  requestFocus(tsl::elm::Element*, tsl::FocusDirection) override {
    return this;
  }

private:
  static constexpr s32 kCardWidth = 328;
  static constexpr s32 kCardHeight = 58;
  static constexpr s32 kCardGap = 5;
  static constexpr s32 kMargin = 12;
  static constexpr std::uint64_t kDamageFadeStartMs = 650;

  static u32 text_width(
    tsl::gfx::Renderer* renderer,
    const std::string& value,
    const float font_size
  ) {
    return renderer
      ->drawString(
        value.c_str(),
        false,
        0,
        0,
        font_size,
        tsl::style::color::ColorTransparent
      )
      .first;
  }

  static void erase_last_codepoint(std::string& value) {
    if (value.empty()) {
      return;
    }
    auto offset = value.size() - 1;
    while (offset > 0 &&
           (static_cast<unsigned char>(value[offset]) & 0xC0) == 0x80) {
      --offset;
    }
    value.erase(offset);
  }

  static std::string fit_text(
    tsl::gfx::Renderer* renderer,
    std::string value,
    const u32 max_width,
    const float font_size
  ) {
    if (text_width(renderer, value, font_size) <= max_width) {
      return value;
    }

    constexpr const char* suffix = "...";
    while (!value.empty()) {
      erase_last_codepoint(value);
      const auto candidate = value + suffix;
      if (text_width(renderer, candidate, font_size) <= max_width) {
        return candidate;
      }
    }
    return suffix;
  }

  static void draw_right_aligned(
    tsl::gfx::Renderer* renderer,
    const std::string& value,
    const s32 right,
    const s32 y,
    const float font_size,
    const tsl::gfx::Color color
  ) {
    const auto width = static_cast<s32>(text_width(renderer, value, font_size));
    renderer->drawString(
      value.c_str(), false, right - width, y, font_size, renderer->a(color)
    );
  }

  static float damage_scale(const std::uint64_t age_ms) {
    if (age_ms < 80) {
      return 0.70F + 0.50F * static_cast<float>(age_ms) / 80.0F;
    }
    if (age_ms < 160) {
      return 1.20F - 0.20F * static_cast<float>(age_ms - 80) / 80.0F;
    }
    return 1.0F;
  }

  static std::uint8_t damage_alpha(const std::uint64_t age_ms) {
    if (age_ms <= kDamageFadeStartMs) {
      return 0xF;
    }
    const auto remaining = mhgu::core::kDamageEventLifetimeMs - age_ms;
    return static_cast<std::uint8_t>(
      std::min<std::uint64_t>(0xF, remaining * 0xF /
                                      (mhgu::core::kDamageEventLifetimeMs -
                                       kDamageFadeStartMs))
    );
  }

  static void draw_damage_events(
    tsl::gfx::Renderer* renderer,
    const mhgu::core::DamageOutput& damage,
    const std::uint64_t now_ms
  ) {
    constexpr std::array<s32, 5> lane_offsets{0, -52, 52, -94, 94};
    constexpr std::array<s32, 5> drift_directions{0, -1, 1, -1, 1};
    constexpr float base_font_size = 38.0F;
    const auto count = std::min(damage.event_count, damage.events.size());
    for (std::size_t index = 0; index < count; ++index) {
      const auto& event = damage.events[index];
      if (now_ms < event.created_at_ms) {
        continue;
      }
      const auto age_ms = now_ms - event.created_at_ms;
      if (age_ms >= mhgu::core::kDamageEventLifetimeMs) {
        continue;
      }

      const auto lane = static_cast<std::size_t>(
        (event.sequence - 1) % lane_offsets.size()
      );
      const auto progress = static_cast<float>(age_ms) /
                            mhgu::core::kDamageEventLifetimeMs;
      const auto inverse = 1.0F - progress;
      const auto eased = 1.0F - inverse * inverse;
      const auto font_size = base_font_size * damage_scale(age_ms);
      const auto center_x = static_cast<s32>(tsl::cfg::FramebufferWidth / 2) +
                            lane_offsets[lane] +
                            static_cast<s32>(
                              drift_directions[lane] * 14.0F * progress
                            );
      const auto baseline_y =
        static_cast<s32>(tsl::cfg::FramebufferHeight * 28 / 100) -
        static_cast<s32>(55.0F * eased);

      char value[16]{};
      std::snprintf(
        value,
        sizeof(value),
        "%u",
        static_cast<unsigned>(event.damage)
      );
      const auto width =
        static_cast<s32>(text_width(renderer, value, font_size));
      const auto left = std::clamp<s32>(
        center_x - width / 2,
        0,
        std::max<s32>(0, tsl::cfg::FramebufferWidth - width)
      );
      const auto alpha = damage_alpha(age_ms);
      renderer->drawString(
        value,
        false,
        left + 2,
        baseline_y + 2,
        font_size,
        renderer->a({0x1, 0x1, 0x1, alpha})
      );
      renderer->drawString(
        value,
        false,
        left,
        baseline_y,
        font_size,
        renderer->a({0xF, 0xC, 0x3, alpha})
      );
    }
  }

  static void draw_status(
    tsl::gfx::Renderer* renderer, const Locale locale, const char* message
  ) {
    constexpr s32 height = 66;
    const s32 y = tsl::cfg::FramebufferHeight - kMargin - height;
    renderer->drawRect(
      kMargin, y, kCardWidth, height, renderer->a({0x1, 0x1, 0x1, 0xB})
    );
    renderer->drawRect(
      kMargin, y, 3, height, renderer->a({0x3, 0xB, 0xA, 0xF})
    );
    renderer->drawString(
      mhgu::core::ui_message(UiMessage::Title, locale),
      false,
      kMargin + 11,
      y + 23,
      17,
      renderer->a({0xF, 0xF, 0xF, 0xF})
    );
    const auto fitted = fit_text(renderer, message, kCardWidth - 22, 15);
    renderer->drawString(
      fitted.c_str(),
      false,
      kMargin + 11,
      y + 51,
      15,
      renderer->a({0xA, 0xA, 0xA, 0xF})
    );
  }

  static void draw_monster(
    tsl::gfx::Renderer* renderer,
    const mhgu::core::MonsterView& monster,
    const Locale locale,
    const s32 x,
    const s32 y
  ) {
    char health[64]{};
    char size[96]{};
    std::snprintf(
      health,
      sizeof(health),
      "%u.%u%% · %u / %u",
      monster.hp_percent_x10 / 10,
      monster.hp_percent_x10 % 10,
      monster.hp,
      monster.max_hp
    );
    std::snprintf(
      size,
      sizeof(size),
      "%s %u%% · %.2f",
      mhgu::core::ui_message(UiMessage::Size, locale),
      monster.size_percent,
      static_cast<double>(monster.actual_size_x100) / 100.0
    );

    renderer->drawRect(
      x, y, kCardWidth, kCardHeight, renderer->a({0x1, 0x1, 0x1, 0xB})
    );
    renderer->drawRect(
      x,
      y,
      3,
      kCardHeight,
      renderer->a(
        monster.crown == mhgu::core::Crown::None
          ? tsl::gfx::Color{0x3, 0xB, 0xA, 0xF}
          : crown_color(monster.crown)
      )
    );

    constexpr s32 content_x = 11;
    constexpr s32 content_width = kCardWidth - content_x * 2;
    constexpr float name_size = 17;
    constexpr float crown_size = 14;
    std::string name = monster.name;
    if (monster.hyper) {
      name += " · ";
      name += mhgu::core::hyper_label(locale);
    }
    const std::string crown = mhgu::core::crown_label(monster.crown, locale);
    const auto crown_width =
      crown.empty() ? u32{0} : text_width(renderer, crown, crown_size);
    const auto name_width =
      static_cast<u32>(content_width) -
      std::min<u32>(crown_width == 0 ? 0 : crown_width + 8, content_width);
    const auto fitted_name =
      fit_text(renderer, std::move(name), name_width, name_size);
    renderer->drawString(
      fitted_name.c_str(),
      false,
      x + content_x,
      y + 20,
      name_size,
      renderer->a({0xF, 0xF, 0xF, 0xF})
    );
    if (!crown.empty()) {
      draw_right_aligned(
        renderer,
        crown,
        x + kCardWidth - content_x,
        y + 20,
        crown_size,
        crown_color(monster.crown)
      );
    }

    const s32 bar_y = y + 25;
    renderer->drawRect(
      x + content_x, bar_y, content_width, 6, renderer->a({0x3, 0x3, 0x3, 0xE})
    );
    const auto hp_width =
      content_width * std::min<std::uint16_t>(monster.hp_percent_x10, 1000) /
      1000;
    const auto hp_color =
      monster.hp_percent_x10 > 500   ? tsl::gfx::Color{0x3, 0xC, 0x7, 0xF}
      : monster.hp_percent_x10 > 200 ? tsl::gfx::Color{0xF, 0xB, 0x3, 0xF}
                                     : tsl::gfx::Color{0xE, 0x4, 0x4, 0xF};
    renderer->drawRect(
      x + content_x, bar_y, hp_width, 6, renderer->a(hp_color)
    );
    const auto size_width = text_width(renderer, size, 14);
    const auto health_width = text_width(renderer, health, 14);
    if (health_width + size_width + 10 > static_cast<u32>(content_width)) {
      std::snprintf(
        health,
        sizeof(health),
        "%u.%u%%",
        monster.hp_percent_x10 / 10,
        monster.hp_percent_x10 % 10
      );
    }
    renderer->drawString(
      health,
      false,
      x + content_x,
      y + 51,
      14,
      renderer->a({0xC, 0xC, 0xC, 0xF})
    );
    draw_right_aligned(
      renderer,
      size,
      x + kCardWidth - content_x,
      y + 51,
      14,
      {0xC, 0xC, 0xC, 0xF}
    );
  }

  Model& model_;
};

class HudGui final : public tsl::Gui {
public:
  explicit HudGui(Model& model)
    : model_(model) {
    FullMode = false;
    alphabackground = 0;
    deactivateOriginalFooter = true;
    TeslaFPS = model_.settings().damage_display_enabled ? 60 : 10;
    tsl::hlp::requestForeground(false);
  }

  ~HudGui() override {
    FullMode = true;
    alphabackground = 0xD;
    deactivateOriginalFooter = false;
    TeslaFPS = 60;
    tsl::hlp::requestForeground(true);
  }

  tsl::elm::Element* createUI() override {
    return new HudElement(model_);
  }

  bool handleInput(
    u64,
    const u64 keys_held,
    const HidTouchState&,
    JoystickPosition,
    JoystickPosition
  ) override {
    if ((keys_held & HidNpadButton_StickL) != 0 &&
        (keys_held & HidNpadButton_StickR) != 0) {
      tsl::goBack();
      return true;
    }
    return false;
  }

private:
  Model& model_;
};

class HunterGui final : public tsl::Gui {
public:
  explicit HunterGui(Model& model)
    : model_(model) {}

  tsl::elm::Element* createUI() override {
    const auto locale = model_.display_locale();
    auto* frame = new LocalizedOverlayFrame(
      mhgu::core::ui_message(UiMessage::Hunter, locale), kVersion
    );
    auto* list = new tsl::elm::List(6);
    list->addItem(runtime_feature_item(
      model_, UiMessage::Invincible, RuntimeFeature::Invincible
    ));
    list->addItem(runtime_feature_item(
      model_, UiMessage::HealthNoDecrease, RuntimeFeature::HealthNoDecrease
    ));
    list->addItem(runtime_feature_item(
      model_, UiMessage::StaminaNoDecrease, RuntimeFeature::StaminaNoDecrease
    ));
    list->addItem(runtime_feature_item(
      model_,
      UiMessage::SharpnessNoDecrease,
      RuntimeFeature::SharpnessNoDecrease
    ));
    list->addItem(runtime_feature_item(
      model_,
      UiMessage::UnlockHunterArtSlots,
      RuntimeFeature::UnlockHunterArtSlots
    ));
    list->addItem(runtime_feature_item(
      model_,
      UiMessage::UnlimitedHunterArts,
      RuntimeFeature::UnlimitedHunterArts
    ));
    list->addItem(runtime_feature_item(
      model_, UiMessage::ValorGaugeNoDecrease,
      RuntimeFeature::ValorGaugeNoDecrease
    ));
    list->addItem(runtime_feature_item(
      model_, UiMessage::AlchemyGaugeFull, RuntimeFeature::AlchemyGaugeFull
    ));
    list->addItem(numeric_feature_item(
      model_,
      UiMessage::LongSwordSpiritGauge,
      NumericFeature::LongSwordSpiritGauge
    ));
    list->addItem(numeric_feature_item(
      model_, UiMessage::SpLevel, NumericFeature::SpLevel
    ));
    list->addItem(runtime_feature_item(
      model_, UiMessage::SpStatusNoExpire, RuntimeFeature::SpStatusNoExpire
    ));
    list->addItem(numeric_feature_item(
      model_, UiMessage::HunterAffinity, NumericFeature::HunterAffinity
    ));
    list->addItem(runtime_feature_item(
      model_, UiMessage::BowgunAutoReload, RuntimeFeature::BowgunAutoReload
    ));
    frame->setContent(list);
    return frame;
  }

  bool handleInput(
    const u64 keys_down,
    u64,
    const HidTouchState&,
    JoystickPosition,
    JoystickPosition
  ) override {
    if ((keys_down & HidNpadButton_B) != 0) {
      tsl::goBack();
      return true;
    }
    return false;
  }

private:
  Model& model_;
};

class CombatParametersGui final : public tsl::Gui {
public:
  explicit CombatParametersGui(Model& model)
    : model_(model) {}

  tsl::elm::Element* createUI() override {
    const auto locale = model_.display_locale();
    auto* frame = new LocalizedOverlayFrame(
      mhgu::core::ui_message(UiMessage::CombatParameters, locale), kVersion
    );
    auto* list = new tsl::elm::List(6);
    list->addItem(monster_damage_mode_item(model_));
    list->addItem(numeric_feature_item(
      model_, UiMessage::AttackMultiplier, NumericFeature::AttackMultiplier
    ));
    list->addItem(numeric_feature_item(
      model_, UiMessage::DefenseMultiplier, NumericFeature::DefenseMultiplier
    ));
    list->addItem(numeric_feature_item(
      model_,
      UiMessage::MovementSpeedMultiplier,
      NumericFeature::MovementSpeedMultiplier
    ));
    frame->setContent(list);
    return frame;
  }

  bool handleInput(
    const u64 keys_down,
    u64,
    const HidTouchState&,
    JoystickPosition,
    JoystickPosition
  ) override {
    if ((keys_down & HidNpadButton_B) != 0) {
      tsl::goBack();
      return true;
    }
    return false;
  }

private:
  Model& model_;
};

class ResourcesGui final : public tsl::Gui {
public:
  explicit ResourcesGui(Model& model)
    : model_(model) {}

  tsl::elm::Element* createUI() override {
    const auto locale = model_.display_locale();
    auto* frame = new LocalizedOverlayFrame(
      mhgu::core::ui_message(UiMessage::Resources, locale), kVersion
    );
    auto* list = new tsl::elm::List(6);
    list->addItem(numeric_feature_item(
      model_, UiMessage::Zenny, NumericFeature::Zenny
    ));
    list->addItem(numeric_feature_item(
      model_, UiMessage::WycademyPoints, NumericFeature::WycademyPoints
    ));
    list->addItem(runtime_feature_item(
      model_,
      UiMessage::ConsumableItemsNoDecrease,
      RuntimeFeature::ConsumableItemsNoDecrease
    ));
    list->addItem(item_pouch_slot_item(model_));
    list->addItem(item_pouch_quantity_item(model_));
    list->addItem(apply_item_pouch_quantity_item(model_));
    frame->setContent(list);
    return frame;
  }

  bool handleInput(
    const u64 keys_down,
    u64,
    const HidTouchState&,
    JoystickPosition,
    JoystickPosition
  ) override {
    if ((keys_down & HidNpadButton_B) != 0) {
      tsl::goBack();
      return true;
    }
    return false;
  }

private:
  Model& model_;
};

class PalicoGui final : public tsl::Gui {
public:
  explicit PalicoGui(Model& model)
    : model_(model) {}

  tsl::elm::Element* createUI() override {
    const auto locale = model_.display_locale();
    auto* frame = new LocalizedOverlayFrame(
      mhgu::core::ui_message(UiMessage::Palico, locale), kVersion
    );
    auto* list = new tsl::elm::List(6);
    list->addItem(runtime_feature_item(
      model_,
      UiMessage::PalicoHealthNoDecrease,
      RuntimeFeature::PalicoHealthNoDecrease
    ));
    list->addItem(numeric_feature_item(
      model_, UiMessage::PalicoAffinity, NumericFeature::PalicoAffinity
    ));
    frame->setContent(list);
    return frame;
  }

  bool handleInput(
    const u64 keys_down,
    u64,
    const HidTouchState&,
    JoystickPosition,
    JoystickPosition
  ) override {
    if ((keys_down & HidNpadButton_B) != 0) {
      tsl::goBack();
      return true;
    }
    return false;
  }

private:
  Model& model_;
};

template <typename Gui>
tsl::elm::ListItem* submenu_item(Model& model, const UiMessage label) {
  auto* item = new tsl::elm::ListItem(text(model, label));
  item->setClickListener([model_ptr = &model](const u64 keys) {
    if ((keys & HidNpadButton_A) == 0) {
      return false;
    }
    tsl::changeTo<Gui>(*model_ptr);
    return true;
  });
  return item;
}

class TransmogGui final : public tsl::Gui {
public:
  explicit TransmogGui(Model& model)
    : model_(model) {}

  tsl::elm::Element* createUI() override {
    const auto locale = model_.display_locale();
    auto* frame = new LocalizedOverlayFrame(
      mhgu::core::ui_message(UiMessage::Transmog, locale), kVersion
    );
    auto* list = new tsl::elm::List(6);
    weapon_item_ = runtime_feature_item(
      model_, UiMessage::WeaponTransmog, RuntimeFeature::WeaponTransmog
    );
    list->addItem(weapon_item_);
    armor_item_ = runtime_feature_item(
      model_, UiMessage::ArmorTransmog, RuntimeFeature::ArmorTransmog
    );
    list->addItem(armor_item_);
    frame->setContent(list);
    return frame;
  }

  bool handleInput(
    const u64 keys_down,
    u64,
    const HidTouchState&,
    JoystickPosition,
    JoystickPosition
  ) override {
    if ((keys_down & HidNpadButton_B) != 0) {
      tsl::goBack();
      return true;
    }
    return false;
  }

private:
  Model& model_;
  tsl::elm::ListItem* weapon_item_{};
  tsl::elm::ListItem* armor_item_{};
};

class MainGui final : public tsl::Gui {
public:
  explicit MainGui(Model& model)
    : model_(model) {}

  tsl::elm::Element* createUI() override {
    refresh_mode();
    const auto locale = model_.display_locale();
    frame_ = new LocalizedOverlayFrame(
      mhgu::core::ui_message(UiMessage::Title, locale), kVersion
    );
    auto* list = new tsl::elm::List(6);

    list->addItem(
      new tsl::elm::CustomDrawer([this](
                                   tsl::gfx::Renderer* renderer,
                                   const u16 x,
                                   const u16 y,
                                   const u16,
                                   const u16
                                 ) {
        const auto view = model_.session_view();
        const auto locale_now = model_.display_locale();
        renderer->drawString(
          status_value(
            view.patch_write_failed ? SessionStatus::WriteFailed
                                    : view.status,
            locale_now
          ),
          false,
          x + 8,
          y + 30,
          19,
          renderer->a({0xF, 0xF, 0xF, 0xF})
        );
        if (view.profile_name != nullptr) {
          renderer->drawString(
            view.profile_name,
            false,
            x + 8,
            y + 58,
            15,
            renderer->a({0x8, 0xB, 0xB, 0xF})
          );
        }
      }),
      70
    );

    language_item_ = new tsl::elm::ListItem(
      mhgu::core::ui_message(UiMessage::Language, locale)
    );
    language_item_->setValue(language_value(model_));
    language_item_->setClickListener([this](const u64 keys) {
      if ((keys & HidNpadButton_A) != 0) {
        model_.cycle_language();
        refresh_labels();
        return true;
      }
      return false;
    });
    list->addItem(language_item_);

    hud_item_ = new tsl::elm::ListItem(
      mhgu::core::ui_message(UiMessage::MonsterInfoOverlay, locale)
    );
    hud_item_->setClickListener([this](const u64 keys) {
      if ((keys & HidNpadButton_A) != 0) {
        tsl::changeTo<HudGui>(model_);
        return true;
      }
      return false;
    });
    list->addItem(hud_item_);

    damage_display_item_ = new tsl::elm::ListItem(
      mhgu::core::ui_message(UiMessage::DamageDisplay, locale)
    );
    damage_display_item_->setValue(mhgu::core::ui_message(
      model_.settings().damage_display_enabled ? UiMessage::On : UiMessage::Off,
      locale
    ));
    damage_display_item_->setClickListener([this](const u64 keys) {
      if ((keys & HidNpadButton_A) != 0) {
        model_.toggle_damage_display();
        refresh_labels();
        return true;
      }
      return false;
    });
    list->addItem(damage_display_item_);

    frame_rate_item_ = new tsl::elm::ListItem(
      mhgu::core::ui_message(UiMessage::FrameRate, locale)
    );
    frame_rate_item_->setValue(frame_rate_value(model_));
    frame_rate_item_->setClickListener([this](const u64 keys) {
      if ((keys & HidNpadButton_A) != 0) {
        model_.cycle_frame_rate();
        refresh_labels();
        return true;
      }
      return false;
    });
    list->addItem(frame_rate_item_);

    preset_item_ = new tsl::elm::ListItem(
      mhgu::core::ui_message(UiMessage::SizePreset, locale)
    );
    preset_item_->setValue(
      mhgu::core::size_preset_label(model_.settings().size_preset, locale)
    );
    preset_item_->setClickListener([this](const u64 keys) {
      if ((keys & HidNpadButton_A) != 0) {
        model_.cycle_size_preset();
        refresh_labels();
        return true;
      }
      return false;
    });
    list->addItem(preset_item_);

    map_item_ = runtime_feature_item(
      model_,
      UiMessage::MapAndLargeMonsters,
      RuntimeFeature::MapAndLargeMonsters
    );
    list->addItem(map_item_);

    carry_item_ = runtime_feature_item(
      model_,
      UiMessage::CarryItemsIntoPouch,
      RuntimeFeature::CarryItemsIntoPouch
    );
    list->addItem(carry_item_);

    transmog_item_ = new tsl::elm::ListItem(
      mhgu::core::ui_message(UiMessage::Transmog, locale)
    );
    transmog_item_->setClickListener([this](const u64 keys) {
      if ((keys & HidNpadButton_A) != 0) {
        tsl::changeTo<TransmogGui>(model_);
        return true;
      }
      return false;
    });
    list->addItem(transmog_item_);

    hunter_item_ = submenu_item<HunterGui>(model_, UiMessage::Hunter);
    list->addItem(hunter_item_);

    combat_parameters_item_ = submenu_item<CombatParametersGui>(
      model_, UiMessage::CombatParameters
    );
    list->addItem(combat_parameters_item_);

    resources_item_ = submenu_item<ResourcesGui>(
      model_, UiMessage::Resources
    );
    list->addItem(resources_item_);

    palico_item_ = submenu_item<PalicoGui>(model_, UiMessage::Palico);
    list->addItem(palico_item_);

    scan_item_ =
      new tsl::elm::ListItem(mhgu::core::ui_message(UiMessage::Scan, locale));
    scan_item_->setClickListener([this](const u64 keys) {
      if ((keys & HidNpadButton_A) != 0) {
        model_.request_rescan();
        return true;
      }
      return false;
    });
    list->addItem(scan_item_);

    frame_->setContent(list);
    return frame_;
  }

  void update() override {
    refresh_mode();
  }

  bool handleInput(
    const u64 keys_down,
    u64,
    const HidTouchState&,
    JoystickPosition,
    JoystickPosition
  ) override {
    if ((keys_down & HidNpadButton_B) != 0) {
      tsl::goBack();
      return true;
    }
    return false;
  }

private:
  void refresh_mode() {
    FullMode = true;
    alphabackground = 0xD;
    deactivateOriginalFooter = false;
    TeslaFPS = 60;
    tsl::hlp::requestForeground(true);
  }

  void refresh_labels() {
    const auto locale = model_.display_locale();
    frame_->setTitle(mhgu::core::ui_message(UiMessage::Title, locale));
    hud_item_->setText(
      mhgu::core::ui_message(UiMessage::MonsterInfoOverlay, locale)
    );
    damage_display_item_->setText(
      mhgu::core::ui_message(UiMessage::DamageDisplay, locale)
    );
    damage_display_item_->setValue(mhgu::core::ui_message(
      model_.settings().damage_display_enabled ? UiMessage::On : UiMessage::Off,
      locale
    ));
    frame_rate_item_->setText(
      mhgu::core::ui_message(UiMessage::FrameRate, locale)
    );
    frame_rate_item_->setValue(frame_rate_value(model_));
    language_item_->setText(
      mhgu::core::ui_message(UiMessage::Language, locale)
    );
    language_item_->setValue(language_value(model_));
    preset_item_->setText(
      mhgu::core::ui_message(UiMessage::SizePreset, locale)
    );
    preset_item_->setValue(
      mhgu::core::size_preset_label(model_.settings().size_preset, locale)
    );
    refresh_runtime_feature_item(
      map_item_,
      model_,
      UiMessage::MapAndLargeMonsters,
      RuntimeFeature::MapAndLargeMonsters
    );
    refresh_runtime_feature_item(
      carry_item_,
      model_,
      UiMessage::CarryItemsIntoPouch,
      RuntimeFeature::CarryItemsIntoPouch
    );
    hunter_item_->setText(mhgu::core::ui_message(UiMessage::Hunter, locale));
    combat_parameters_item_->setText(
      mhgu::core::ui_message(UiMessage::CombatParameters, locale)
    );
    resources_item_->setText(
      mhgu::core::ui_message(UiMessage::Resources, locale)
    );
    palico_item_->setText(mhgu::core::ui_message(UiMessage::Palico, locale));
    transmog_item_->setText(
      mhgu::core::ui_message(UiMessage::Transmog, locale)
    );
    scan_item_->setText(mhgu::core::ui_message(UiMessage::Scan, locale));
  }

  Model& model_;
  LocalizedOverlayFrame* frame_{};
  tsl::elm::ListItem* hud_item_{};
  tsl::elm::ListItem* damage_display_item_{};
  tsl::elm::ListItem* language_item_{};
  tsl::elm::ListItem* frame_rate_item_{};
  tsl::elm::ListItem* preset_item_{};
  tsl::elm::ListItem* map_item_{};
  tsl::elm::ListItem* carry_item_{};
  tsl::elm::ListItem* transmog_item_{};
  tsl::elm::ListItem* hunter_item_{};
  tsl::elm::ListItem* combat_parameters_item_{};
  tsl::elm::ListItem* resources_item_{};
  tsl::elm::ListItem* palico_item_{};
  tsl::elm::ListItem* scan_item_{};
};

class MhguOverlay final : public tsl::Overlay {
public:
  void initServices() override {
    model_.start();
  }

  void exitServices() override {
    model_.stop();
  }

  std::unique_ptr<tsl::Gui> loadInitialGui() override {
    return initially<MainGui>(model_);
  }

private:
  Model model_;
};

}  // namespace

int main(const int argc, char** argv) {
  return tsl::loop<MhguOverlay>(argc, argv);
}
