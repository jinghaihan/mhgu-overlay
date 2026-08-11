#pragma once

// Included by source/ui/main.cpp inside its private UI namespace.

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

class QuestGui final : public tsl::Gui {
public:
  explicit QuestGui(Model& model)
    : model_(model) {}

  tsl::elm::Element* createUI() override {
    const auto locale = model_.display_locale();
    frame_ = new LocalizedOverlayFrame(
      mhgu::core::ui_message(UiMessage::Quest, locale), kVersion
    );
    auto* list = new tsl::elm::List(6);

    infinite_time_item_ = new tsl::elm::ListItem(
      mhgu::core::ui_message(UiMessage::InfiniteQuestTime, locale)
    );
    infinite_time_item_->setClickListener([this](const u64 keys) {
      if ((keys & HidNpadButton_A) == 0) {
        return false;
      }
      model_.toggle_infinite_quest_time();
      refresh_items();
      return true;
    });
    list->addItem(infinite_time_item_);

    unlimited_faints_item_ = new tsl::elm::ListItem(
      mhgu::core::ui_message(UiMessage::UnlimitedFaints, locale)
    );
    unlimited_faints_item_->setClickListener([this](const u64 keys) {
      if ((keys & HidNpadButton_A) == 0) {
        return false;
      }
      model_.toggle_unlimited_faints();
      refresh_items();
      return true;
    });
    list->addItem(unlimited_faints_item_);

    complete_quest_item_ = new tsl::elm::ListItem(
      mhgu::core::ui_message(UiMessage::CompleteQuest, locale)
    );
    complete_quest_item_->setClickListener([this](const u64 keys) {
      if ((keys & HidNpadButton_A) == 0) {
        return false;
      }
      model_.request_complete_quest();
      refresh_items();
      return true;
    });
    list->addItem(complete_quest_item_);

    refresh_items();
    frame_->setContent(list);
    return frame_;
  }

  void update() override {
    refresh_items();
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
  const char* completion_value(const Locale locale) const {
    switch (model_.quest_completion_status()) {
      case QuestCompletionStatus::Pending:
        return "...";
      case QuestCompletionStatus::Completed:
        return mhgu::core::ui_message(UiMessage::Completed, locale);
      case QuestCompletionStatus::NoActiveQuest:
        return mhgu::core::ui_message(UiMessage::NoActiveQuest, locale);
      case QuestCompletionStatus::Failed:
        return mhgu::core::ui_message(UiMessage::Failed, locale);
      default:
        return mhgu::core::ui_message(UiMessage::Execute, locale);
    }
  }

  void refresh_items() {
    if (frame_ == nullptr || infinite_time_item_ == nullptr ||
        unlimited_faints_item_ == nullptr || complete_quest_item_ == nullptr) {
      return;
    }
    const auto locale = model_.display_locale();
    const auto settings = model_.settings();
    frame_->setTitle(mhgu::core::ui_message(UiMessage::Quest, locale));
    infinite_time_item_->setText(
      mhgu::core::ui_message(UiMessage::InfiniteQuestTime, locale)
    );
    infinite_time_item_->setValue(mhgu::core::ui_message(
      settings.infinite_quest_time ? UiMessage::On : UiMessage::Off,
      locale
    ));
    unlimited_faints_item_->setText(
      mhgu::core::ui_message(UiMessage::UnlimitedFaints, locale)
    );
    unlimited_faints_item_->setValue(mhgu::core::ui_message(
      settings.unlimited_faints ? UiMessage::On : UiMessage::Off,
      locale
    ));
    complete_quest_item_->setText(
      mhgu::core::ui_message(UiMessage::CompleteQuest, locale)
    );
    complete_quest_item_->setValue(completion_value(locale));
  }

  Model& model_;
  LocalizedOverlayFrame* frame_{};
  tsl::elm::ListItem* infinite_time_item_{};
  tsl::elm::ListItem* unlimited_faints_item_{};
  tsl::elm::ListItem* complete_quest_item_{};
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
