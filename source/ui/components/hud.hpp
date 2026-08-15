#pragma once

// Included by source/ui/main.cpp inside its private UI namespace.

enum class HudContent : std::uint8_t {
  MonsterCards,
  DamageOnly,
};

class HudElement final : public tsl::elm::Element {
public:
  HudElement(Model& model, const HudContent content)
    : model_(model), content_(content) {}

  void draw(tsl::gfx::Renderer* renderer) override {
    renderer->clearScreen();
    const auto view = model_.session_view();
    const auto settings = model_.settings();
    const auto locale = model_.display_locale();
    const auto count = std::min<std::size_t>(
      view.output.monster_count, mhgu::core::kMaxMonsters
    );

    if (content_ == HudContent::MonsterCards) {
      if (count == 0) {
        const auto* message =
          view.status == SessionStatus::Ready
            ? mhgu::core::ui_message(UiMessage::NoMonsters, locale)
            : status_value(view.status, locale);
        draw_status(renderer, locale, message, settings.hud_layout);
      } else {
        for (std::size_t index = 0; index < count; ++index) {
          const auto position = card_position(
            settings.hud_layout, count, index
          );
          draw_monster(
            renderer,
            view.output.monsters[index],
            locale,
            position.x,
            position.y
          );
        }
      }
    }
    if (content_ == HudContent::DamageOnly ||
        settings.damage_display_enabled) {
      draw_damage_events(
        renderer,
        view.damage,
        monotonic_milliseconds(),
        settings.hud_layout,
        count
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
  struct HudPosition {
    s32 x;
    s32 y;
  };

  static constexpr s32 kCardWidth = 360;
  static constexpr s32 kCardHeight = 58;
  static constexpr s32 kCardGap = 5;
  static constexpr s32 kMargin = 12;
  static constexpr std::uint64_t kDamageFadeStartMs = 650;
  static constexpr s32 kDamageStaggerStep = 45;
  static constexpr s32 kDamageDriftRadius = 80;
  static constexpr s32 kDamageOverlapPadding = 10;
  static constexpr std::size_t kTopCenterColumns = 3;

  struct DamageRenderEvent {
    mhgu::core::DamageEvent event{};
    std::uint64_t age_ms{};
    float font_size{};
    s32 width{};
    s32 left{};
    s32 baseline_y{};
    char value[16]{};
  };

  static HudPosition card_position(
    const HudLayout layout, const std::size_t count, const std::size_t index
  ) {
    const auto step = kCardHeight + kCardGap;
    if (layout == HudLayout::BottomLeftVertical) {
      const auto stack_height =
        static_cast<s32>(count * kCardHeight + (count - 1) * kCardGap);
      return {
        kMargin,
        tsl::cfg::FramebufferHeight - kMargin - stack_height +
          static_cast<s32>(index * step),
      };
    }
    if (layout == HudLayout::TopRightVertical) {
      return {
        tsl::cfg::FramebufferWidth - kMargin - kCardWidth,
        kMargin + static_cast<s32>(index * step),
      };
    }
    if (layout == HudLayout::CenterLeftVertical ||
        layout == HudLayout::CenterRightVertical) {
      const auto stack_height =
        static_cast<s32>(count * kCardHeight + (count - 1) * kCardGap);
      return {
        layout == HudLayout::CenterRightVertical
          ? tsl::cfg::FramebufferWidth - kMargin - kCardWidth
          : kMargin,
        (tsl::cfg::FramebufferHeight - stack_height) / 2 +
          static_cast<s32>(index * step),
      };
    }

    const auto columns = std::min(kTopCenterColumns, count);
    const auto row = index / columns;
    const auto row_start = row * columns;
    const auto row_count = std::min(columns, count - row_start);
    const auto column = index - row_start;
    const auto row_width =
      static_cast<s32>(row_count * kCardWidth + (row_count - 1) * kCardGap);
    return {
      (tsl::cfg::FramebufferWidth - row_width) / 2 +
        static_cast<s32>(column * (kCardWidth + kCardGap)),
      kMargin + static_cast<s32>(row * step),
    };
  }

  static HudPosition status_position(const HudLayout layout) {
    switch (layout) {
      case HudLayout::TopRightVertical:
        return {
          tsl::cfg::FramebufferWidth - kMargin - kCardWidth,
          kMargin,
        };
      case HudLayout::TopCenterHorizontal:
        return {
          (tsl::cfg::FramebufferWidth - kCardWidth) / 2,
          kMargin,
        };
      case HudLayout::CenterLeftVertical:
        return {
          kMargin,
          (tsl::cfg::FramebufferHeight - 66) / 2,
        };
      case HudLayout::CenterRightVertical:
        return {
          tsl::cfg::FramebufferWidth - kMargin - kCardWidth,
          (tsl::cfg::FramebufferHeight - 66) / 2,
        };
      default:
        return {
          kMargin,
          tsl::cfg::FramebufferHeight - kMargin - 66,
        };
    }
  }

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

  static s32 damage_candidate_offset(const std::size_t candidate) {
    if (candidate == 0) {
      return 0;
    }
    const auto distance = static_cast<s32>((candidate + 1) / 2) *
                          kDamageStaggerStep;
    return (candidate & 1U) != 0 ? -distance : distance;
  }

  static s32 damage_candidate_direction(const s32 offset) {
    return offset < 0 ? -1 : offset > 0 ? 1 : 0;
  }

  static s32 damage_drift(
    const DamageRenderEvent& event, const s32 direction
  ) {
    if (direction == 0) {
      return 0;
    }
    const auto progress = static_cast<float>(event.age_ms) /
                          mhgu::core::kDamageEventLifetimeMs;
    return direction * static_cast<s32>(kDamageDriftRadius * progress);
  }

  static bool damage_ranges_overlap(
    const s32 left_a,
    const s32 right_a,
    const s32 left_b,
    const s32 right_b
  ) {
    return left_a < right_b + kDamageOverlapPadding &&
           left_b < right_a + kDamageOverlapPadding;
  }

  static void draw_damage_text(
    tsl::gfx::Renderer* renderer,
    const char* value,
    const s32 left,
    const s32 baseline_y,
    const float font_size,
    const std::uint8_t alpha
  ) {
    if (damage_text_renderer().draw(
          renderer, value, left, baseline_y, font_size, alpha
        )) {
      return;
    }

    // Keep damage readable if the shared system font cannot be initialized.
    renderer->drawString(
      value,
      false,
      left + 3,
      baseline_y + 3,
      font_size,
      renderer->a({0x0, 0x0, 0x0, alpha})
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

  static void draw_damage_events(
    tsl::gfx::Renderer* renderer,
    const mhgu::core::DamageOutput& damage,
    const std::uint64_t now_ms,
    const HudLayout layout,
    const std::size_t monster_count
  ) {
    constexpr float base_font_size = 38.0F;
    std::array<DamageRenderEvent, mhgu::core::kMaxDamageEvents> active{};
    std::size_t active_count{};
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

      auto& render_event = active[active_count++];
      render_event.event = event;
      render_event.age_ms = age_ms;
      render_event.font_size = base_font_size * damage_scale(age_ms);
      std::snprintf(
        render_event.value,
        sizeof(render_event.value),
        "%u",
        static_cast<unsigned>(event.damage)
      );
      render_event.width = damage_text_renderer().measure(
        render_event.value, render_event.font_size
      );
      if (render_event.width <= 0) {
        render_event.width = static_cast<s32>(text_width(
          renderer, render_event.value, render_event.font_size
        ));
      }

      const auto progress = static_cast<float>(age_ms) /
                            mhgu::core::kDamageEventLifetimeMs;
      const auto inverse = 1.0F - progress;
      const auto eased = 1.0F - inverse * inverse;
      const auto damage_height_percent =
        layout == HudLayout::TopCenterHorizontal && monster_count > 6 ? 52
                                                                       : 28;
      render_event.baseline_y =
        static_cast<s32>(
          tsl::cfg::FramebufferHeight * damage_height_percent / 100
        ) -
        static_cast<s32>(55.0F * eased);
    }

    // Place newer events first so the newest hit remains anchored at center.
    // Each following event takes the nearest free slot around that anchor.
    for (std::size_t reverse = active_count; reverse > 0; --reverse) {
      auto& render_event = active[reverse - 1];
      const auto candidate_count = active_count * 2 + 1;
      for (std::size_t candidate = 0;
           candidate < candidate_count;
           ++candidate) {
        const auto offset = damage_candidate_offset(candidate);
        const auto direction = damage_candidate_direction(offset);
        const auto center_x =
          static_cast<s32>(tsl::cfg::FramebufferWidth / 2) + offset +
          damage_drift(render_event, direction);
        const auto left = std::clamp<s32>(
          center_x - render_event.width / 2,
          0,
          std::max<s32>(0, tsl::cfg::FramebufferWidth - render_event.width)
        );
        const auto right = left + render_event.width;
        bool overlaps{};
        for (std::size_t placed = reverse; placed < active_count; ++placed) {
          const auto placed_left = active[placed].left;
          const auto placed_right = placed_left + active[placed].width;
          if (damage_ranges_overlap(left, right, placed_left, placed_right)) {
            overlaps = true;
            break;
          }
        }
        if (!overlaps) {
          render_event.left = left;
          break;
        }
      }
    }

    for (std::size_t index = 0; index < active_count; ++index) {
      const auto& render_event = active[index];
      const auto alpha = damage_alpha(render_event.age_ms);
      draw_damage_text(
        renderer,
        render_event.value,
        render_event.left,
        render_event.baseline_y,
        render_event.font_size,
        alpha
      );
    }
  }

  static void draw_status(
    tsl::gfx::Renderer* renderer,
    const Locale locale,
    const char* message,
    const HudLayout layout
  ) {
    constexpr s32 height = 66;
    const auto position = status_position(layout);
    const s32 x = position.x;
    const s32 y = position.y;
    renderer->drawRect(
      x, y, kCardWidth, height, renderer->a({0x1, 0x1, 0x1, 0xB})
    );
    renderer->drawRect(
      x, y, 3, height, renderer->a({0x3, 0xB, 0xA, 0xF})
    );
    renderer->drawString(
      mhgu::core::ui_message(UiMessage::Title, locale),
      false,
      x + 11,
      y + 23,
      17,
      renderer->a({0xF, 0xF, 0xF, 0xF})
    );
    const auto fitted = fit_text(renderer, message, kCardWidth - 22, 15);
    renderer->drawString(
      fitted.c_str(),
      false,
      x + 11,
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
  const HudContent content_;
};

class TransparentHudGui : public tsl::Gui {
public:
  TransparentHudGui(Model& model, const HudContent content)
    : model_(model), content_(content) {
    const auto damage_only = content_ == HudContent::DamageOnly;
    const auto needs_damage_rate =
      damage_only || model_.settings().damage_display_enabled;
    model_.set_monster_hud_active(!damage_only);
    model_.set_damage_only_hud_active(damage_only);
    FullMode = false;
    alphabackground = 0;
    deactivateOriginalFooter = true;
    TeslaFPS = needs_damage_rate ? 60 : 10;
    tsl::hlp::requestForeground(false);
  }

  ~TransparentHudGui() override {
    model_.set_monster_hud_active(false);
    model_.set_damage_only_hud_active(false);
    FullMode = true;
    alphabackground = 0xD;
    deactivateOriginalFooter = false;
    TeslaFPS = 30;
    tsl::hlp::requestForeground(true);
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

protected:
  Model& model_;
  const HudContent content_;
};

class HudGui final : public TransparentHudGui {
public:
  explicit HudGui(Model& model)
    : TransparentHudGui(model, HudContent::MonsterCards) {}

  tsl::elm::Element* createUI() override {
    return new HudElement(model_, content_);
  }
};

class DamageOnlyGui final : public TransparentHudGui {
public:
  explicit DamageOnlyGui(Model& model)
    : TransparentHudGui(model, HudContent::DamageOnly) {}

  tsl::elm::Element* createUI() override {
    return new HudElement(model_, content_);
  }
};
