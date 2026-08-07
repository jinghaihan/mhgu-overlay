#define TESLA_INIT_IMPL
#include <tesla.hpp>

#include <algorithm>
#include <cstdio>
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
using mhgu::core::RuntimeFeature;
using mhgu::core::SizePreset;
using mhgu::core::UiMessage;
using mhgu::platform::switch_adapter::SessionStatus;

#ifndef MHGU_OVERLAY_VERSION
#define MHGU_OVERLAY_VERSION "development"
#endif

constexpr const char* kVersion = "v" MHGU_OVERLAY_VERSION;

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

tsl::elm::CustomDrawer* section_header(
  Model& model, const UiMessage message
) {
  return new tsl::elm::CustomDrawer(
    [model_ptr = &model, message](
      tsl::gfx::Renderer* renderer,
      const u16 x,
      const u16 y,
      const u16,
      const u16
    ) {
      renderer->drawRect(
        x + 8, y + 13, 3, 19, renderer->a({0x3, 0xB, 0xA, 0xF})
      );
      renderer->drawString(
        text(*model_ptr, message),
        false,
        x + 19,
        y + 31,
        17,
        renderer->a({0xA, 0xD, 0xD, 0xF})
      );
    }
  );
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
      return;
    }

    const auto stack_height =
      static_cast<s32>(count * kCardHeight + (count - 1) * kCardGap);
    const s32 first_y = tsl::cfg::FramebufferHeight - kMargin - stack_height;
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
    TeslaFPS = 10;
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

class BattleGui final : public tsl::Gui {
public:
  explicit BattleGui(Model& model)
    : model_(model) {}

  tsl::elm::Element* createUI() override {
    const auto locale = model_.display_locale();
    auto* frame = new LocalizedOverlayFrame(
      mhgu::core::ui_message(UiMessage::BattleFunctions, locale), kVersion
    );
    auto* list = new tsl::elm::List(6);
    list->addItem(section_header(model_, UiMessage::Hunter), 44);

    invincible_item_ = runtime_feature_item(
      model_, UiMessage::Invincible, RuntimeFeature::Invincible
    );
    list->addItem(invincible_item_);

    health_item_ = runtime_feature_item(
      model_, UiMessage::HealthNoDecrease, RuntimeFeature::HealthNoDecrease
    );
    list->addItem(health_item_);

    stamina_item_ = runtime_feature_item(
      model_, UiMessage::StaminaNoDecrease, RuntimeFeature::StaminaNoDecrease
    );
    list->addItem(stamina_item_);

    sharpness_item_ = runtime_feature_item(
      model_,
      UiMessage::SharpnessNoDecrease,
      RuntimeFeature::SharpnessNoDecrease
    );
    list->addItem(sharpness_item_);

    hunter_art_slots_item_ = runtime_feature_item(
      model_,
      UiMessage::UnlockHunterArtSlots,
      RuntimeFeature::UnlockHunterArtSlots
    );
    list->addItem(hunter_art_slots_item_);

    hunter_arts_item_ = runtime_feature_item(
      model_,
      UiMessage::UnlimitedHunterArts,
      RuntimeFeature::UnlimitedHunterArts
    );
    list->addItem(hunter_arts_item_);

    valor_item_ = runtime_feature_item(
      model_, UiMessage::ValorGaugeNoDecrease,
      RuntimeFeature::ValorGaugeNoDecrease
    );
    list->addItem(valor_item_);

    alchemy_item_ = runtime_feature_item(
      model_, UiMessage::AlchemyGaugeFull, RuntimeFeature::AlchemyGaugeFull
    );
    list->addItem(alchemy_item_);

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
  tsl::elm::ListItem* invincible_item_{};
  tsl::elm::ListItem* health_item_{};
  tsl::elm::ListItem* stamina_item_{};
  tsl::elm::ListItem* sharpness_item_{};
  tsl::elm::ListItem* hunter_art_slots_item_{};
  tsl::elm::ListItem* hunter_arts_item_{};
  tsl::elm::ListItem* valor_item_{};
  tsl::elm::ListItem* alchemy_item_{};
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
          status_value(view.status, locale_now),
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

    hud_item_ = new tsl::elm::ListItem(
      mhgu::core::ui_message(UiMessage::OpenOverlay, locale)
    );
    hud_item_->setClickListener([this](const u64 keys) {
      if ((keys & HidNpadButton_A) != 0) {
        tsl::changeTo<HudGui>(model_);
        return true;
      }
      return false;
    });
    list->addItem(hud_item_);

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

    battle_item_ = new tsl::elm::ListItem(
      mhgu::core::ui_message(UiMessage::BattleFunctions, locale)
    );
    battle_item_->setClickListener([this](const u64 keys) {
      if ((keys & HidNpadButton_A) != 0) {
        tsl::changeTo<BattleGui>(model_);
        return true;
      }
      return false;
    });
    list->addItem(battle_item_);

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
      mhgu::core::ui_message(UiMessage::OpenOverlay, locale)
    );
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
    battle_item_->setText(
      mhgu::core::ui_message(UiMessage::BattleFunctions, locale)
    );
    scan_item_->setText(mhgu::core::ui_message(UiMessage::Scan, locale));
  }

  Model& model_;
  LocalizedOverlayFrame* frame_{};
  tsl::elm::ListItem* hud_item_{};
  tsl::elm::ListItem* language_item_{};
  tsl::elm::ListItem* frame_rate_item_{};
  tsl::elm::ListItem* preset_item_{};
  tsl::elm::ListItem* map_item_{};
  tsl::elm::ListItem* carry_item_{};
  tsl::elm::ListItem* battle_item_{};
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
