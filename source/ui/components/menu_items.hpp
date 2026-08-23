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

tsl::elm::ListItem* hud_content_item(Model& model) {
  auto* item = new tsl::elm::ListItem(text(model, UiMessage::HudContent));
  refresh_hud_content_item(item, model);
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

tsl::elm::ListItem* food_skill_item(
  Model& model, const std::size_t slot
) {
  auto* item = new tsl::elm::ListItem(text(model, kFoodSkillLabels[slot]));
  refresh_food_skill_item(item, model, slot);
  item->setClickListener(
    [model_ptr = &model, item, slot](const u64 keys) {
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
};
