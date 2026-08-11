#pragma once

// Included by source/ui/main.cpp inside its private UI namespace.

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

    hud_layout_item_ = hud_layout_item(model_);
    list->addItem(hud_layout_item_);

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

    quest_item_ = submenu_item<QuestGui>(model_, UiMessage::Quest);
    list->addItem(quest_item_);

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
    TeslaFPS = 30;
  }

  void refresh_labels() {
    const auto locale = model_.display_locale();
    frame_->setTitle(mhgu::core::ui_message(UiMessage::Title, locale));
    hud_item_->setText(
      mhgu::core::ui_message(UiMessage::MonsterInfoOverlay, locale)
    );
    refresh_hud_layout_item(hud_layout_item_, model_);
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
    quest_item_->setText(mhgu::core::ui_message(UiMessage::Quest, locale));
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
  tsl::elm::ListItem* hud_layout_item_{};
  tsl::elm::ListItem* damage_display_item_{};
  tsl::elm::ListItem* language_item_{};
  tsl::elm::ListItem* frame_rate_item_{};
  tsl::elm::ListItem* preset_item_{};
  tsl::elm::ListItem* map_item_{};
  tsl::elm::ListItem* carry_item_{};
  tsl::elm::ListItem* transmog_item_{};
  tsl::elm::ListItem* hunter_item_{};
  tsl::elm::ListItem* combat_parameters_item_{};
  tsl::elm::ListItem* quest_item_{};
  tsl::elm::ListItem* resources_item_{};
  tsl::elm::ListItem* palico_item_{};
  tsl::elm::ListItem* scan_item_{};
};
