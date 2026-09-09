#pragma once

// Included by source/ui/main.cpp inside its private UI namespace.

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

const char* hud_layout_value(Model& model) {
  switch (model.settings().hud_layout) {
    case HudLayout::TopRightVertical:
      return text(model, UiMessage::HudTopRightVertical);
    case HudLayout::TopCenterHorizontal:
      return text(model, UiMessage::HudTopCenterHorizontal);
    case HudLayout::CenterLeftVertical:
      return text(model, UiMessage::HudCenterLeftVertical);
    case HudLayout::CenterRightVertical:
      return text(model, UiMessage::HudCenterRightVertical);
    default:
      return text(model, UiMessage::HudBottomLeftVertical);
  }
}

void refresh_hud_layout_item(tsl::elm::ListItem* item, Model& model) {
  item->setText(text(model, UiMessage::HudLayout));
  item->setValue(hud_layout_value(model));
}

const char* hud_content_value(Model& model) {
  switch (model.settings().hud_content) {
    case HudContent::MonsterInfoAndDamage:
      return text(model, UiMessage::HudMonsterInfoAndDamage);
    case HudContent::DamageOnly:
      return text(model, UiMessage::HudDamageOnly);
    default:
      return text(model, UiMessage::HudMonsterInfo);
  }
}

void refresh_hud_content_item(tsl::elm::ListItem* item, Model& model) {
  item->setText(text(model, UiMessage::HudContent));
  item->setValue(hud_content_value(model));
}

// Damage display settings submenu. Defined after LocalizedOverlayFrame below.
class DamageDisplayGui;

tsl::elm::ListItem* hud_content_item(Model& model) {
  auto* item = new tsl::elm::ListItem(text(model, UiMessage::HudContent));
  refresh_hud_content_item(item, model);
  item->setClickListener(
    [model_ptr = &model, item](const u64 keys) {
      // A opens the damage display settings submenu; the value itself can
      // still be cycled quickly with Left / Right.
      if ((keys & HidNpadButton_A) != 0) {
        tsl::changeTo<DamageDisplayGui>(*model_ptr);
        return true;
      }
      int direction{};
      if ((keys & HidNpadButton_Left) != 0) {
        direction = -1;
      } else if ((keys & HidNpadButton_Right) != 0) {
        direction = 1;
      } else {
        return false;
      }
      model_ptr->cycle_hud_content(direction);
      refresh_hud_content_item(item, *model_ptr);
      return true;
    }
  );
  return item;
}

tsl::elm::ListItem* hud_layout_item(Model& model) {
  auto* item = new tsl::elm::ListItem(text(model, UiMessage::HudLayout));
  refresh_hud_layout_item(item, model);
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
      model_ptr->cycle_hud_layout(direction);
      refresh_hud_layout_item(item, *model_ptr);
      return true;
    }
  );
  return item;
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
    [model_ptr = &model, item, label, feature](const u64 keys) {
      if ((keys & HidNpadButton_A) == 0) {
        return false;
      }
      model_ptr->toggle_runtime_feature(feature);
      refresh_runtime_feature_item(item, *model_ptr, label, feature);
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

constexpr std::array<UiMessage, mhgu::core::kFoodSkillSlotCount>
  kFoodSkillLabels{{
    UiMessage::FoodSkill1,
    UiMessage::FoodSkill2,
    UiMessage::FoodSkill3,
  }};

void refresh_food_skill_item(
  tsl::elm::ListItem* item, Model& model, const std::size_t slot
) {
  if (slot >= kFoodSkillLabels.size()) {
    return;
  }
  const auto skill = model.settings().food_skills[slot];
  char value[96]{};
  std::snprintf(
    value,
    sizeof(value),
    "%02X %s",
    static_cast<unsigned>(skill),
    mhgu::core::food_skill_name(skill, model.display_locale())
  );
  item->setText(text(model, kFoodSkillLabels[slot]));
  item->setValue(value);
}

// Picker that lists every food skill (0x01..0x41) and assigns the chosen one
// to a given slot.  Defined after LocalizedOverlayFrame below.
class FoodSkillPickerGui;

tsl::elm::ListItem* food_skill_item(
  Model& model, const std::size_t slot
) {
  auto* item = new tsl::elm::ListItem(text(model, kFoodSkillLabels[slot]));
  refresh_food_skill_item(item, model, slot);
  item->setClickListener(
    [model_ptr = &model, item, slot](const u64 keys) {
      if ((keys & HidNpadButton_A) != 0) {
        tsl::changeTo<FoodSkillPickerGui>(*model_ptr, slot);
        return true;
      }
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
      model_ptr->adjust_food_skill(slot, delta);
      refresh_food_skill_item(item, *model_ptr, slot);
      return true;
    }
  );
  return item;
}

const char* food_skill_apply_value(Model& model) {
  const auto locale = model.display_locale();
  switch (model.food_skill_apply_status()) {
    case FoodSkillApplyStatus::Pending:
      return "...";
    case FoodSkillApplyStatus::Applied:
      return mhgu::core::ui_message(UiMessage::Completed, locale);
    case FoodSkillApplyStatus::NoCharacterData:
      return mhgu::core::ui_message(UiMessage::NoCharacterData, locale);
    case FoodSkillApplyStatus::Failed:
      return mhgu::core::ui_message(UiMessage::Failed, locale);
    default:
      return mhgu::core::ui_message(UiMessage::Execute, locale);
  }
}

void refresh_apply_food_skills_item(
  tsl::elm::ListItem* item, Model& model
) {
  item->setText(text(model, UiMessage::ApplyFoodSkills));
  item->setValue(food_skill_apply_value(model));
}

tsl::elm::ListItem* apply_food_skills_item(Model& model) {
  auto* item = new tsl::elm::ListItem(text(model, UiMessage::ApplyFoodSkills));
  refresh_apply_food_skills_item(item, model);
  item->setClickListener(
    [model_ptr = &model, item](const u64 keys) {
      if ((keys & HidNpadButton_A) == 0) {
        return false;
      }
      model_ptr->request_food_skills_write();
      refresh_apply_food_skills_item(item, *model_ptr);
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
        model_ptr->toggle_numeric_feature(feature);
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

  // Draw the menu on the left half of the screen only.  The full 1280px
  // framebuffer is needed for the hunting HUD's side-by-side layout, but the
  // menu list only needs half that width and leaves the rest of the game
  // visible.
  void draw(tsl::gfx::Renderer* renderer) override {
    const u16 half_width = tsl::cfg::FramebufferWidth / 2;

    renderer->drawRect(
      0, 0, half_width, tsl::cfg::FramebufferHeight,
      tsl::gfx::Color{0x0, 0x0, 0x0, alphabackground}
    );

    renderer->drawString(
      m_title.c_str(), false, 20, 50, 30,
      tsl::gfx::Color{0xF, 0xF, 0xF, 0xF}
    );
    renderer->drawString(
      m_subtitle.c_str(), false, 20, 70, 15,
      tsl::gfx::Color{0xC, 0xC, 0xC, 0xF}
    );

    if (FullMode) {
      renderer->drawRect(
        15, tsl::cfg::FramebufferHeight - 73, half_width - 30, 1,
        tsl::gfx::Color{0xF, 0xF, 0xF, 0xF}
      );
    }
    if (!deactivateOriginalFooter) {
      renderer->drawString(
        "\uE0E1  Back     \uE0E0  OK", false, 30, 693, 23,
        tsl::gfx::Color{0xF, 0xF, 0xF, 0xF}
      );
    }

    if (m_contentElement != nullptr) {
      m_contentElement->frame(renderer);
    }
  }

  void layout(
    u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight
  ) override {
    const u16 half_width = parentWidth / 2;
    setBoundaries(parentX, parentY, half_width, parentHeight);

    if (m_contentElement != nullptr) {
      m_contentElement->setBoundaries(
        parentX + 35, parentY + 140, half_width - 85, parentHeight - 73 - 105
      );
      m_contentElement->invalidate();
    }
  }
};

// Lists every food skill (0x01..0x41) and assigns the chosen one to a slot.
// Returning to FoodSkillsGui lets its update() refresh the slot label.
class FoodSkillPickerGui final : public tsl::Gui {
public:
  FoodSkillPickerGui(Model& model, const std::size_t slot)
    : model_(model), slot_(slot) {}

  tsl::elm::Element* createUI() override {
    const auto locale = model_.display_locale();
    frame_ = new LocalizedOverlayFrame(
      mhgu::core::ui_message(UiMessage::FoodSkills, locale), kVersion
    );
    list_ = new tsl::elm::List(6);
    current_index_ = static_cast<std::size_t>(
      model_.food_skill(slot_) - mhgu::core::kMinimumFoodSkillId
    );
    for (unsigned id = mhgu::core::kMinimumFoodSkillId;
         id <= mhgu::core::kMaximumFoodSkillId; ++id) {
      const auto skill_id = static_cast<mhgu::core::FoodSkillId>(id);
      char label[96]{};
      std::snprintf(
        label,
        sizeof(label),
        "%02X %s",
        id,
        mhgu::core::food_skill_name(skill_id, locale)
      );
      auto* item = new tsl::elm::ListItem(label);
      item->setClickListener(
        [this, skill_id](const u64 keys) {
          if ((keys & HidNpadButton_A) == 0) {
            return false;
          }
          model_.set_food_skill(slot_, skill_id);
          tsl::goBack();
          return true;
        }
      );
      list_->addItem(item);
    }
    frame_->setContent(list_);
    return frame_;
  }

  void update() override {
    // Move focus to the currently assigned skill on the first frame so the
    // list opens at the right row instead of always jumping to the top.
    if (focus_initialized_) {
      return;
    }
    focus_initialized_ = true;
    for (std::size_t i = 0; i < current_index_; ++i) {
      this->requestFocus(list_, tsl::FocusDirection::Down);
    }
  }

  bool handleInput(
    const u64 keys_down,
    const u64 keys_held,
    const HidTouchState&,
    JoystickPosition,
    JoystickPosition
  ) override {
    if (handle_minimize_combo(keys_down, keys_held)) {
      return true;
    }
    if ((keys_down & HidNpadButton_B) != 0) {
      tsl::goBack();
      return true;
    }
    return false;
  }

private:
  Model& model_;
  std::size_t slot_;
  LocalizedOverlayFrame* frame_{};
  tsl::elm::List* list_{};
  std::size_t current_index_{};
  bool focus_initialized_{false};
};

const char* damage_drift_mode_label(Model& model) {
  switch (model.settings().damage_display.drift_mode) {
    case DamageDriftMode::Upward:
      return text(model, UiMessage::DriftUpward);
    case DamageDriftMode::Off:
      return text(model, UiMessage::Off);
    default:
      return text(model, UiMessage::DriftRandom);
  }
}

const char* damage_appear_effect_label(Model& model) {
  switch (model.settings().damage_display.appear_effect) {
    case DamageAppearEffect::SizeOnly:
      return text(model, UiMessage::AppearSizeOnly);
    case DamageAppearEffect::ColorOnly:
      return text(model, UiMessage::AppearColorOnly);
    case DamageAppearEffect::Fixed:
      return text(model, UiMessage::AppearFixed);
    default:
      return text(model, UiMessage::AppearSizeAndColor);
  }
}

const char* damage_stagger_mode_label(Model& model) {
  switch (model.settings().damage_display.stagger_mode) {
    case DamageStaggerMode::Horizontal:
      return text(model, UiMessage::StaggerHorizontal);
    case DamageStaggerMode::Upward:
      return text(model, UiMessage::StaggerUpward);
    case DamageStaggerMode::Down:
      return text(model, UiMessage::StaggerDown);
    case DamageStaggerMode::Left:
      return text(model, UiMessage::StaggerLeft);
    case DamageStaggerMode::Right:
      return text(model, UiMessage::StaggerRight);
    default:
      return text(model, UiMessage::StaggerVertical);
  }
}

const char* damage_stagger_type_label(Model& model) {
  switch (model.settings().damage_display.stagger_type) {
    case DamageStaggerType::Linear:
      return text(model, UiMessage::StaggerLinear);
    default:
      return text(model, UiMessage::StaggerZigzag);
  }
}

// Damage display effect settings.  Row 1 mirrors the outer "HUD 显示内容"
// item so the two stay in sync; the remaining rows tune the damage numbers
// (overlap, size, position, appear effect, drift); the last row restores the
// tuned defaults.
class DamageDisplayGui final : public tsl::Gui {
public:
  explicit DamageDisplayGui(Model& model)
    : model_(model) {}

  tsl::elm::Element* createUI() override {
    const auto locale = model_.display_locale();
    frame_ = new LocalizedOverlayFrame(
      mhgu::core::ui_message(UiMessage::DamageDisplay, locale), kVersion
    );
    auto* list = new tsl::elm::List(6);

    auto* mode_item = new tsl::elm::ListItem(
      text(model_, UiMessage::HudDisplayMode)
    );
    mode_item->setClickListener(
      [this](const u64 keys) {
        int direction{};
        if ((keys & HidNpadButton_Left) != 0) {
          direction = -1;
        } else if ((keys & (HidNpadButton_A | HidNpadButton_Right)) != 0) {
          direction = 1;
        } else {
          return false;
        }
        model_.cycle_hud_content(direction);
        return true;
      }
    );
    list->addItem(mode_item);

    auto* overlap_item = new tsl::elm::ListItem(
      text(model_, UiMessage::DamageOverlap)
    );
    overlap_item->setClickListener(
      [this](const u64 keys) {
        if ((keys & HidNpadButton_A) == 0) {
          return false;
        }
        model_.toggle_damage_overlap();
        return true;
      }
    );
    list->addItem(overlap_item);

    auto* size_item = new tsl::elm::ListItem(
      text(model_, UiMessage::DamageSize)
    );
    size_item->setClickListener(
      [this](const u64 keys) {
        const auto delta = adjust_delta(keys, 5, 20);
        if (delta == 0) {
          return false;
        }
        model_.adjust_damage_size(delta);
        return true;
      }
    );
    list->addItem(size_item);

    auto* position_item = new tsl::elm::ListItem(
      text(model_, UiMessage::DamagePosition)
    );
    position_item->setClickListener(
      [this](const u64 keys) {
        const auto delta = adjust_delta(keys, 5, 10);
        if (delta == 0) {
          return false;
        }
        model_.adjust_damage_position(delta);
        return true;
      }
    );
    list->addItem(position_item);

    auto* appear_item = new tsl::elm::ListItem(
      text(model_, UiMessage::DamageAppearEffect)
    );
    appear_item->setClickListener(
      [this](const u64 keys) {
        int direction{};
        if ((keys & HidNpadButton_Left) != 0) {
          direction = -1;
        } else if ((keys & (HidNpadButton_A | HidNpadButton_Right)) != 0) {
          direction = 1;
        } else {
          return false;
        }
        model_.cycle_damage_appear_effect(direction);
        return true;
      }
    );
    list->addItem(appear_item);

    auto* drift_item = new tsl::elm::ListItem(
      text(model_, UiMessage::DriftMode)
    );
    drift_item->setClickListener(
      [this](const u64 keys) {
        int direction{};
        if ((keys & HidNpadButton_Left) != 0) {
          direction = -1;
        } else if ((keys & (HidNpadButton_A | HidNpadButton_Right)) != 0) {
          direction = 1;
        } else {
          return false;
        }
        model_.cycle_damage_drift_mode(direction);
        return true;
      }
    );
    list->addItem(drift_item);

    auto* distance_item = new tsl::elm::ListItem(
      text(model_, UiMessage::DriftDistance)
    );
    distance_item->setClickListener(
      [this](const u64 keys) {
        const auto delta = adjust_delta(keys, 5, 10);
        if (delta == 0) {
          return false;
        }
        model_.adjust_damage_drift_distance(delta);
        return true;
      }
    );
    list->addItem(distance_item);

    auto* speed_item = new tsl::elm::ListItem(
      text(model_, UiMessage::DriftSpeed)
    );
    speed_item->setClickListener(
      [this](const u64 keys) {
        const auto delta = adjust_delta(keys, 5, 20);
        if (delta == 0) {
          return false;
        }
        model_.adjust_damage_drift_speed(delta);
        return true;
      }
    );
    list->addItem(speed_item);

    auto* stagger_item = new tsl::elm::ListItem(
      text(model_, UiMessage::StaggerMode)
    );
    stagger_item->setClickListener(
      [this](const u64 keys) {
        int direction{};
        if ((keys & HidNpadButton_Left) != 0) {
          direction = -1;
        } else if ((keys & (HidNpadButton_A | HidNpadButton_Right)) != 0) {
          direction = 1;
        } else {
          return false;
        }
        model_.cycle_damage_stagger_mode(direction);
        return true;
      }
    );
    list->addItem(stagger_item);

    auto* stagger_type_item = new tsl::elm::ListItem(
      text(model_, UiMessage::StaggerType)
    );
    stagger_type_item->setClickListener(
      [this](const u64 keys) {
        int direction{};
        if ((keys & HidNpadButton_Left) != 0) {
          direction = -1;
        } else if ((keys & (HidNpadButton_A | HidNpadButton_Right)) != 0) {
          direction = 1;
        } else {
          return false;
        }
        model_.cycle_damage_stagger_type(direction);
        return true;
      }
    );
    list->addItem(stagger_type_item);

    auto* reset_item = new tsl::elm::ListItem(
      text(model_, UiMessage::ResetDamageDisplay)
    );
    reset_item->setClickListener(
      [this](const u64 keys) {
        if ((keys & HidNpadButton_A) == 0) {
          return false;
        }
        model_.reset_damage_display();
        return true;
      }
    );
    list->addItem(reset_item);

    items_[0] = mode_item;
    items_[1] = overlap_item;
    items_[2] = size_item;
    items_[3] = position_item;
    items_[4] = appear_item;
    items_[5] = drift_item;
    items_[6] = distance_item;
    items_[7] = speed_item;
    items_[8] = stagger_item;
    items_[9] = stagger_type_item;

    frame_->setContent(list);
    return frame_;
  }

  void update() override {
    if (frame_ == nullptr) {
      return;
    }
    frame_->setTitle(
      mhgu::core::ui_message(UiMessage::DamageDisplay, model_.display_locale())
    );
    items_[0]->setValue(hud_content_value(model_));
    items_[1]->setValue(
      text(model_, model_.settings().damage_display.overlap
                     ? UiMessage::On
                     : UiMessage::Off)
    );
    char value[16]{};
    const auto& display = model_.settings().damage_display;
    std::snprintf(value, sizeof(value), "%u%%", display.size_percent);
    items_[2]->setValue(value);
    std::snprintf(value, sizeof(value), "%u%%", display.position_percent);
    items_[3]->setValue(value);
    items_[4]->setValue(damage_appear_effect_label(model_));
    items_[5]->setValue(damage_drift_mode_label(model_));
    std::snprintf(value, sizeof(value), "%u", display.drift_distance);
    items_[6]->setValue(value);
    std::snprintf(value, sizeof(value), "%u%%", display.drift_speed_percent);
    items_[7]->setValue(value);
    items_[8]->setValue(damage_stagger_mode_label(model_));
    items_[9]->setValue(damage_stagger_type_label(model_));
  }

  bool handleInput(
    const u64 keys_down,
    const u64 keys_held,
    const HidTouchState&,
    JoystickPosition,
    JoystickPosition
  ) override {
    if (handle_minimize_combo(keys_down, keys_held)) {
      return true;
    }
    if ((keys_down & HidNpadButton_B) != 0) {
      tsl::goBack();
      return true;
    }
    return false;
  }

private:
  static int adjust_delta(const u64 keys, const int small, const int large) {
    if ((keys & HidNpadButton_Left) != 0) {
      return -small;
    }
    if ((keys & HidNpadButton_Right) != 0) {
      return small;
    }
    if ((keys & HidNpadButton_L) != 0) {
      return -large;
    }
    if ((keys & HidNpadButton_R) != 0) {
      return large;
    }
    return 0;
  }

  Model& model_;
  LocalizedOverlayFrame* frame_{};
  std::array<tsl::elm::ListItem*, 10> items_{};
};
