#pragma once

#include <cmath>

// Included by source/ui/main.cpp inside its private UI namespace.

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

    const auto shows_monster_info =
      settings.hud_content != HudContent::DamageOnly;
    const auto shows_damage =
      settings.hud_content != HudContent::MonsterInfo;

    if (shows_monster_info) {
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
    if (shows_damage) {
      draw_damage_events(
        renderer,
        view.damage,
        monotonic_milliseconds(),
        settings.hud_layout,
        shows_monster_info ? count : 0,
        settings.damage_display
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
  static constexpr s32 kDamageStaggerStep = 28;
  static constexpr s32 kDamageHorizontalStep = 15;
  static constexpr s32 kDamageOverlapPadding = 8;
  static constexpr std::size_t kTopCenterColumns = 3;

  struct DamageOffset {
    s32 dx{};
    s32 dy{};
  };

  struct DamageRenderEvent {
    mhgu::core::DamageEvent event{};
    std::uint64_t age_ms{};
    float font_size{};
    s32 width{};
    s32 base_width{};  // width measured at base (unscaled) font size
    s32 left{};
    s32 baseline_y{};
    char value[16]{};
    s32 drift_x{};
    s32 drift_y{};
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
    // Pop small → overshoot large → settle, matching the original feel.
    if (age_ms < 80) {
      return 0.70F + 1.30F * static_cast<float>(age_ms) / 80.0F;
    }
    if (age_ms < 160) {
      return 2.00F - 1.00F * static_cast<float>(age_ms - 80) / 80.0F;
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

  // Whitish pale yellow while the number pops up to the peak, then a quick
  // 40 ms blend back to the original yellow once the scale starts settling.
  struct DamageColor {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
  };

  static constexpr DamageColor kDamagePaleColor{0xFF, 0xF2, 0xB4};
  static constexpr DamageColor kDamageNormalColor{0xFF, 0xCC, 0x33};

  static DamageColor damage_color(const std::uint64_t age_ms) {
    constexpr DamageColor pale = kDamagePaleColor;
    constexpr DamageColor normal = kDamageNormalColor;
    if (age_ms < 80) {
      return pale;
    }
    if (age_ms < 120) {
      const auto t = static_cast<float>(age_ms - 80) / 40.0F;
      const auto mix = [t](const std::uint8_t from, const std::uint8_t to) {
        return static_cast<std::uint8_t>(
          static_cast<float>(from) + t * static_cast<float>(to - from)
        );
      };
      return {
        mix(pale.r, normal.r),
        mix(pale.g, normal.g),
        mix(pale.b, normal.b),
      };
    }
    return normal;
  }

  // Deterministic pseudo-random angle in [0, 2π) derived from the event
  // sequence, so each damage number drifts along its own fixed direction
  // without jittering between frames.
  static float damage_random_angle(const std::uint64_t sequence) {
    const auto hash =
      sequence * 6364136223846793005ULL + 1442695040888963407ULL;
    const auto degrees = static_cast<float>(hash % 360U);
    return degrees * 3.14159265358979323846F / 180.0F;
  }

  // Drift is delayed while the scale-in animation runs (0 → 2.0 → 1.0,
  // kDamageScaleMs = 160ms) so the number holds still while it pops, then
  // begins drifting outward once the scale settles at 1.0.  Without the
  // scale animation (fixed-size appear effects) there is nothing to wait
  // for, so drift starts immediately.  The speed multiplier scales the
  // drift progress rate.
  static s32 damage_drift_distance(
    const std::uint64_t age_ms,
    const std::uint8_t max_drift,
    const std::uint8_t speed_percent,
    const bool scale_animated
  ) {
    constexpr std::uint64_t kScaleAnimationMs = 160;
    const auto delay_ms = scale_animated ? kScaleAnimationMs : 0;
    if (age_ms < delay_ms) {
      return 0;
    }
    const auto effective_age = age_ms - delay_ms;
    const auto speed = static_cast<float>(speed_percent) / 100.0F;
    const auto progress =
      (static_cast<float>(effective_age) /
       mhgu::core::kDamageEventLifetimeMs) *
      speed;
    return static_cast<s32>(
      static_cast<float>(max_drift) * std::clamp(progress, 0.0F, 1.0F)
    );
  }

  // Stagger functions: search for a free slot around the center to avoid
  // overlapping damage numbers.  Three modes cover different layouts:
  //   - VerticalMixed: primarily up/down with a small horizontal zig-zag
  //   - Horizontal: primarily left/right with a small vertical zig-zag
  //   - Upward: stack upward only (newest stays at bottom/center)

  static DamageOffset damage_candidate_vertical_mixed(
    const std::size_t candidate, const s32 v_step
  ) {
    if (candidate == 0) {
      return {0, 0};
    }
    const auto row = (candidate + 1) / 2;
    const auto is_up = (candidate & 1U) != 0;
    const auto dy = is_up ? -static_cast<s32>(row) * v_step
                          : static_cast<s32>(row) * v_step;
    const auto row_sign = (row & 1U) != 0 ? 1 : -1;
    const auto dx = is_up ? row_sign * kDamageHorizontalStep
                          : -row_sign * kDamageHorizontalStep;
    return {dx, dy};
  }

  static DamageOffset damage_candidate_horizontal(
    const std::size_t candidate, const s32 h_step
  ) {
    if (candidate == 0) {
      return {0, 0};
    }
    const auto col = (candidate + 1) / 2;
    const auto is_left = (candidate & 1U) != 0;
    const auto dx = is_left
                      ? -static_cast<s32>(col) * h_step
                      : static_cast<s32>(col) * h_step;
    const auto col_sign = (col & 1U) != 0 ? 1 : -1;
    const auto dy = is_left ? -col_sign * (kDamageStaggerStep / 2)
                            : col_sign * (kDamageStaggerStep / 2);
    return {dx, dy};
  }

  static DamageOffset damage_candidate_upward(
    const std::size_t candidate, const s32 v_step
  ) {
    if (candidate == 0) {
      return {0, 0};
    }
    const auto dx = (candidate & 1U) != 0 ? -kDamageHorizontalStep
                                          : kDamageHorizontalStep;
    return {dx, -static_cast<s32>(candidate) * v_step};
  }

  static DamageOffset damage_candidate_down(
    const std::size_t candidate, const s32 v_step
  ) {
    if (candidate == 0) {
      return {0, 0};
    }
    const auto dx = (candidate & 1U) != 0 ? -kDamageHorizontalStep
                                          : kDamageHorizontalStep;
    return {dx, static_cast<s32>(candidate) * v_step};
  }

  static DamageOffset damage_candidate_left(
    const std::size_t candidate, const s32 h_step
  ) {
    if (candidate == 0) {
      return {0, 0};
    }
    const auto dx = -static_cast<s32>(candidate) * h_step;
    const auto dy = (candidate & 1U) != 0 ? -kDamageStaggerStep / 2
                                          : kDamageStaggerStep / 2;
    return {dx, dy};
  }

  static DamageOffset damage_candidate_right(
    const std::size_t candidate, const s32 h_step
  ) {
    if (candidate == 0) {
      return {0, 0};
    }
    const auto dx = static_cast<s32>(candidate) * h_step;
    const auto dy = (candidate & 1U) != 0 ? -kDamageStaggerStep / 2
                                          : kDamageStaggerStep / 2;
    return {dx, dy};
  }

  // Linear variants: same alternation as zigzag but without the
  // perpendicular micro-offset.  For bidirectional modes this means
  // alternating up/down or left/right in a straight line; for
  // unidirectional modes it means alternating the perpendicular axis
  // while moving in the primary direction (same as zigzag since the
  // perpendicular IS the alternation, not a micro).
  static DamageOffset damage_candidate_vertical_linear(
    const std::size_t candidate, const s32 step
  ) {
    if (candidate == 0) {
      return {0, 0};
    }
    const auto row = (candidate + 1) / 2;
    const auto is_up = (candidate & 1U) != 0;
    const auto dy = is_up ? -static_cast<s32>(row) * step
                          : static_cast<s32>(row) * step;
    return {0, dy};
  }

  static DamageOffset damage_candidate_horizontal_linear(
    const std::size_t candidate, const s32 step
  ) {
    if (candidate == 0) {
      return {0, 0};
    }
    const auto col = (candidate + 1) / 2;
    const auto is_left = (candidate & 1U) != 0;
    const auto dx = is_left
                      ? -static_cast<s32>(col) * step
                      : static_cast<s32>(col) * step;
    return {dx, 0};
  }

  static DamageOffset damage_candidate_upward_linear(
    const std::size_t candidate, const s32 step
  ) {
    if (candidate == 0) {
      return {0, 0};
    }
    return {0, -static_cast<s32>(candidate) * step};
  }

  static DamageOffset damage_candidate_down_linear(
    const std::size_t candidate, const s32 step
  ) {
    if (candidate == 0) {
      return {0, 0};
    }
    return {0, static_cast<s32>(candidate) * step};
  }

  static DamageOffset damage_candidate_left_linear(
    const std::size_t candidate, const s32 step
  ) {
    if (candidate == 0) {
      return {0, 0};
    }
    return {-static_cast<s32>(candidate) * step, 0};
  }

  static DamageOffset damage_candidate_right_linear(
    const std::size_t candidate, const s32 step
  ) {
    if (candidate == 0) {
      return {0, 0};
    }
    return {static_cast<s32>(candidate) * step, 0};
  }

  static DamageOffset damage_stagger_offset(
    const DamageStaggerMode mode,
    const DamageStaggerType type,
    const std::size_t candidate,
    const s32 font_height,
    const s32 event_width
  ) {
    // Step must exceed the number dimensions to avoid overlap between
    // consecutive candidates on the same axis.  This applies to both
    // zigzag and linear modes.  The padding must be large enough to
    // accommodate the scale animation peak (2.0x) for the brief 160ms
    // after a number appears, otherwise numbers would overlap during
    // the pop-in animation.
    const auto v_step = std::max(kDamageStaggerStep, font_height + 16);
    const auto h_step = std::max(
      kDamageHorizontalStep * 2, event_width + 8
    );
    if (type == DamageStaggerType::Linear) {
      switch (mode) {
        case DamageStaggerMode::Horizontal:
          return damage_candidate_horizontal_linear(candidate, h_step);
        case DamageStaggerMode::Upward:
          return damage_candidate_upward_linear(candidate, v_step);
        case DamageStaggerMode::Down:
          return damage_candidate_down_linear(candidate, v_step);
        case DamageStaggerMode::Left:
          return damage_candidate_left_linear(candidate, h_step);
        case DamageStaggerMode::Right:
          return damage_candidate_right_linear(candidate, h_step);
        case DamageStaggerMode::VerticalMixed:
        default:
          return damage_candidate_vertical_linear(candidate, v_step);
      }
    }
    switch (mode) {
      case DamageStaggerMode::Horizontal:
        return damage_candidate_horizontal(candidate, h_step);
      case DamageStaggerMode::Upward:
        return damage_candidate_upward(candidate, v_step);
      case DamageStaggerMode::Down:
        return damage_candidate_down(candidate, v_step);
      case DamageStaggerMode::Left:
        return damage_candidate_left(candidate, h_step);
      case DamageStaggerMode::Right:
        return damage_candidate_right(candidate, h_step);
      default:
        return damage_candidate_vertical_mixed(candidate, v_step);
    }
  }

  static bool damage_rects_overlap(
    const s32 left_a,
    const s32 right_a,
    const s32 top_a,
    const s32 bottom_a,
    const s32 left_b,
    const s32 right_b,
    const s32 top_b,
    const s32 bottom_b
  ) {
    return left_a < right_b + kDamageOverlapPadding &&
           left_b < right_a + kDamageOverlapPadding &&
           top_a < bottom_b + kDamageOverlapPadding &&
           top_b < bottom_a + kDamageOverlapPadding;
  }

  static void draw_damage_text(
    tsl::gfx::Renderer* renderer,
    const char* value,
    const s32 left,
    const s32 baseline_y,
    const float font_size,
    const std::uint8_t alpha,
    const DamageColor color
  ) {
    if (damage_text_renderer().draw(
          renderer,
          value,
          left,
          baseline_y,
          font_size,
          alpha,
          color.r,
          color.g,
          color.b
        )) {
      return;
    }

    // Keep damage readable if the shared system font cannot be initialized.
    const auto nibble = [](const std::uint8_t channel) {
      return static_cast<std::uint8_t>((channel + 8) / 17);
    };
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
      renderer->a({nibble(color.r), nibble(color.g), nibble(color.b), alpha})
    );
  }

  static void draw_damage_events(
    tsl::gfx::Renderer* renderer,
    const mhgu::core::DamageOutput& damage,
    const std::uint64_t now_ms,
    const HudLayout layout,
    const std::size_t monster_count,
    const mhgu::core::DamageDisplaySettings& display
  ) {
    const float base_font_size =
      38.0F * static_cast<float>(display.size_percent) / 100.0F;
    std::array<DamageRenderEvent, mhgu::core::kMaxDamageEvents> active{};
    std::size_t active_count{};
    s32 base_baseline_y{};
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
      // The appear effect decides which of the two spawn animations run:
      // the size pop (0.7 → 2.0 → 1.0) and/or the pale → yellow color blend.
      const auto scale_animated =
        display.appear_effect == DamageAppearEffect::SizeAndColor ||
        display.appear_effect == DamageAppearEffect::SizeOnly;
      render_event.font_size =
        base_font_size * (scale_animated ? damage_scale(age_ms) : 1.0F);
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
      // Measure width at the base (unscaled) font size for stable stagger
      // step calculation.  Using the scaled width would make h_step vary
      // per-event (2x at peak vs 1x at rest), causing asymmetric spacing.
      render_event.base_width = damage_text_renderer().measure(
        render_event.value, base_font_size
      );
      if (render_event.base_width <= 0) {
        render_event.base_width = static_cast<s32>(text_width(
          renderer, render_event.value, base_font_size
        ));
      }

      // Numbers sit at the configured height; when the top-center layout is
      // crowded the row drops lower so it stays clear of the monster cards.
      auto damage_height_percent =
        static_cast<s32>(display.position_percent);
      if (layout == HudLayout::TopCenterHorizontal && monster_count > 6) {
        damage_height_percent = std::min(damage_height_percent + 24, 90);
      }
      // Numbers spawn at the center and immediately drift outward along the
      // configured mode.
      base_baseline_y = static_cast<s32>(
        tsl::cfg::FramebufferHeight * damage_height_percent / 100
      );
      const auto drift = damage_drift_distance(
        age_ms, display.drift_distance, display.drift_speed_percent,
        scale_animated
      );
      if (display.drift_mode == DamageDriftMode::Random) {
        const auto angle = damage_random_angle(event.sequence);
        render_event.drift_x = static_cast<s32>(drift * std::cos(angle));
        render_event.drift_y = static_cast<s32>(drift * std::sin(angle));
      } else if (display.drift_mode == DamageDriftMode::Upward) {
        render_event.drift_y = -drift;
      }
      render_event.baseline_y = base_baseline_y + render_event.drift_y;
    }

    if (display.overlap) {
      // Numbers may stack: draw each at its natural spawn + drift position
      // without any avoidance search.
      for (std::size_t index = 0; index < active_count; ++index) {
        auto& render_event = active[index];
        const auto center_x =
          static_cast<s32>(tsl::cfg::FramebufferWidth / 2) +
          render_event.drift_x;
        render_event.left = std::clamp<s32>(
          center_x - render_event.width / 2,
          0,
          std::max<s32>(0, tsl::cfg::FramebufferWidth - render_event.width)
        );
      }
    } else {
      // No overlap: assign each event a fixed slot by age index so
      // positions are stable — a number's slot never changes when new
      // numbers arrive, preventing the left-right jitter caused by
      // re-searching every frame.  The newest event is at candidate 0
      // (center), the oldest at candidate 1, the second-oldest at
      // candidate 2, etc.
      //
      // Drift is added on top of the fixed stagger slot, exactly like in
      // overlap mode: each number keeps its slot but still drifts outward
      // with age, so the drift distance / speed / mode settings take effect
      // with overlap disabled too.  New numbers spawn with zero drift, so
      // the slot spacing is preserved at the moment a number appears and
      // older numbers simply spread further out as they fade.
      for (std::size_t index = 0; index < active_count; ++index) {
        auto& render_event = active[index];
        const auto candidate = active_count - 1 - index;
        const auto offset = damage_stagger_offset(
          display.stagger_mode, display.stagger_type, candidate,
          static_cast<s32>(base_font_size),
          render_event.base_width
        );
        const auto center_x =
          static_cast<s32>(tsl::cfg::FramebufferWidth / 2) + offset.dx +
          render_event.drift_x;
        const auto baseline_y =
          base_baseline_y + offset.dy + render_event.drift_y;
        render_event.left = std::clamp<s32>(
          center_x - render_event.width / 2,
          0,
          std::max<s32>(0, tsl::cfg::FramebufferWidth - render_event.width)
        );
        render_event.baseline_y = baseline_y;
      }
    }

    const auto color_animated =
      display.appear_effect == DamageAppearEffect::SizeAndColor ||
      display.appear_effect == DamageAppearEffect::ColorOnly;
    for (std::size_t index = 0; index < active_count; ++index) {
      const auto& render_event = active[index];
      const auto alpha = damage_alpha(render_event.age_ms);
      const auto color = color_animated
                           ? damage_color(render_event.age_ms)
                           : kDamageNormalColor;
      draw_damage_text(
        renderer,
        render_event.value,
        render_event.left,
        render_event.baseline_y,
        render_event.font_size,
        alpha,
        color
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
};

class HudGui final : public tsl::Gui {
public:
  explicit HudGui(Model& model)
    : model_(model) {
    const auto shows_damage =
      model_.settings().hud_content != HudContent::MonsterInfo;
    model_.set_monster_hud_active(true);
    FullMode = false;
    alphabackground = 0;
    deactivateOriginalFooter = true;
    TeslaFPS = shows_damage ? 60 : 10;
    tsl::hlp::requestForeground(false);
  }

  ~HudGui() override {
    model_.set_monster_hud_active(false);
    FullMode = true;
    alphabackground = 0xD;
    deactivateOriginalFooter = false;
    TeslaFPS = 30;
    tsl::hlp::requestForeground(true);
  }

  bool handleInput(
    const u64 /*keys_down*/,
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

  tsl::elm::Element* createUI() override {
    return new HudElement(model_);
  }

private:
  Model& model_;
};
