#define TESLA_INIT_IMPL
#include <tesla.hpp>

#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>

#include "mhgu/app/model.hpp"
#include "mhgu/core/locale.hpp"
#include "mhgu/core/messages.hpp"

namespace {

using mhgu::app::Model;
using mhgu::core::Locale;
using mhgu::core::LocaleMode;
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
        case LocaleMode::English: return "English";
        case LocaleMode::SimplifiedChinese: return "简体中文";
        case LocaleMode::Japanese: return "日本語";
        default: return text(model, UiMessage::Automatic);
    }
}

const char* status_value(
    const SessionStatus status,
    const Locale locale
) {
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
        case mhgu::core::Crown::Mini: return {0x4, 0xB, 0xF, 0xF};
        case mhgu::core::Crown::Silver: return {0xC, 0xD, 0xE, 0xF};
        case mhgu::core::Crown::Gold: return {0xF, 0xC, 0x3, 0xF};
        default: return {0xF, 0xF, 0xF, 0xF};
    }
}

class HudElement final : public tsl::elm::Element {
public:
    explicit HudElement(Model& model) : model_(model) {}

    void draw(tsl::gfx::Renderer* renderer) override {
        const auto view = model_.session_view();
        const auto locale = model_.display_locale();
        const auto count = std::min<std::size_t>(view.output.monster_count, 3);
        const auto panel_height = static_cast<s32>(
            count == 0 ? 110 : 58 + count * 104
        );
        const s32 panel_x = 12;
        const s32 panel_y = 720 - panel_height - 14;
        const s32 panel_width = tsl::cfg::FramebufferWidth - 24;

        renderer->drawRect(
            panel_x,
            panel_y,
            panel_width,
            panel_height,
            renderer->a({0x1, 0x1, 0x1, 0xC})
        );
        renderer->drawRect(
            panel_x,
            panel_y,
            4,
            panel_height,
            renderer->a({0x3, 0xB, 0xA, 0xF})
        );
        renderer->drawString(
            mhgu::core::ui_message(UiMessage::Title, locale),
            false,
            panel_x + 16,
            panel_y + 30,
            20,
            renderer->a({0xF, 0xF, 0xF, 0xF})
        );

        if (count == 0) {
            renderer->drawString(
                status_value(view.status, locale),
                false,
                panel_x + 16,
                panel_y + 72,
                18,
                renderer->a({0xA, 0xA, 0xA, 0xF})
            );
            return;
        }

        for (std::size_t index = 0; index < count; ++index) {
            draw_monster(
                renderer,
                view.output.monsters[index],
                locale,
                panel_x + 16,
                panel_y + 54 + static_cast<s32>(index * 104),
                panel_width - 32
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

    tsl::elm::Element* requestFocus(
        tsl::elm::Element*,
        tsl::FocusDirection
    ) override {
        return this;
    }

private:
    static void draw_monster(
        tsl::gfx::Renderer* renderer,
        const mhgu::core::MonsterView& monster,
        const Locale locale,
        const s32 x,
        const s32 y,
        const s32 width
    ) {
        char health[64]{};
        char size[96]{};
        std::snprintf(
            health,
            sizeof(health),
            "%u / %u",
            monster.hp,
            monster.max_hp
        );
        std::snprintf(
            size,
            sizeof(size),
            "%s %u%%  %.2f",
            mhgu::core::ui_message(UiMessage::Size, locale),
            monster.size_percent,
            static_cast<double>(monster.actual_size_x100) / 100.0
        );

        std::string name = monster.name;
        if (monster.hyper) {
            name += " · ";
            name += mhgu::core::hyper_label(locale);
        }
        renderer->drawString(
            name.c_str(),
            false,
            x,
            y + 22,
            21,
            renderer->a({0xF, 0xF, 0xF, 0xF})
        );
        const auto* crown = mhgu::core::crown_label(monster.crown, locale);
        if (crown[0] != '\0') {
            renderer->drawString(
                crown,
                false,
                x + width - 105,
                y + 22,
                17,
                renderer->a(crown_color(monster.crown))
            );
        }

        const s32 bar_y = y + 34;
        renderer->drawRect(
            x,
            bar_y,
            width,
            10,
            renderer->a({0x3, 0x3, 0x3, 0xE})
        );
        renderer->drawRect(
            x,
            bar_y,
            width * monster.hp_percent_x10 / 1000,
            10,
            renderer->a({0x3, 0xC, 0x7, 0xF})
        );
        renderer->drawString(
            health,
            false,
            x,
            y + 70,
            17,
            renderer->a({0xC, 0xC, 0xC, 0xF})
        );
        renderer->drawString(
            size,
            false,
            x + width - 190,
            y + 70,
            17,
            renderer->a({0xC, 0xC, 0xC, 0xF})
        );
    }

    Model& model_;
};

class HudGui final : public tsl::Gui {
public:
    explicit HudGui(Model& model) : model_(model) {
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
        touchPosition,
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

class MainGui final : public tsl::Gui {
public:
    explicit MainGui(Model& model) : model_(model) {}

    tsl::elm::Element* createUI() override {
        refresh_mode();
        const auto locale = model_.display_locale();
        auto* frame = new tsl::elm::OverlayFrame(
            mhgu::core::ui_message(UiMessage::Title, locale),
            kVersion
        );
        auto* list = new tsl::elm::List(6);

        list->addItem(
            new tsl::elm::CustomDrawer(
                [this](
                    tsl::gfx::Renderer* renderer,
                    const s32 x,
                    const s32 y,
                    const s32,
                    const s32
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
                },
                70
            )
        );

        hud_item_ = new tsl::elm::ListItem(
            mhgu::core::ui_message(UiMessage::Hud, locale)
        );
        hud_item_->setClickListener([this](const u64 keys) {
            if ((keys & HidNpadButton_A) != 0) {
                tsl::changeTo<HudGui>(model_);
                return true;
            }
            return false;
        });
        list->addItem(hud_item_);

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

        preset_item_ = new tsl::elm::ListItem(
            mhgu::core::ui_message(UiMessage::SizePreset, locale)
        );
        preset_item_->setValue(
            mhgu::core::size_preset_label(
                model_.settings().size_preset,
                locale
            )
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

        lock_item_ = new tsl::elm::ToggleListItem(
            mhgu::core::ui_message(UiMessage::SizeLock, locale),
            model_.settings().size_lock_armed,
            mhgu::core::ui_message(UiMessage::On, locale),
            mhgu::core::ui_message(UiMessage::Off, locale)
        );
        lock_item_->setStateChangedListener([this](const bool state) {
            model_.set_size_lock(state);
        });
        list->addItem(lock_item_);

        auto* scan_item = new tsl::elm::ListItem(
            mhgu::core::ui_message(UiMessage::Scan, locale)
        );
        scan_item->setClickListener([this](const u64 keys) {
            if ((keys & HidNpadButton_A) != 0) {
                model_.request_rescan();
                return true;
            }
            return false;
        });
        list->addItem(scan_item);

        list->addItem(
            new tsl::elm::CustomDrawer(
                [this](
                    tsl::gfx::Renderer* renderer,
                    const s32 x,
                    const s32 y,
                    const s32,
                    const s32
                ) {
                    renderer->drawString(
                        text(model_, UiMessage::BackHint),
                        false,
                        x + 8,
                        y + 28,
                        15,
                        renderer->a({0x9, 0x9, 0x9, 0xF})
                    );
                },
                52
            )
        );

        frame->setContent(list);
        return frame;
    }

    void update() override {
        refresh_mode();
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
        language_item_->setText(
            mhgu::core::ui_message(UiMessage::Language, locale)
        );
        language_item_->setValue(language_value(model_));
        preset_item_->setText(
            mhgu::core::ui_message(UiMessage::SizePreset, locale)
        );
        preset_item_->setValue(
            mhgu::core::size_preset_label(
                model_.settings().size_preset,
                locale
            )
        );
        lock_item_->setText(
            mhgu::core::ui_message(UiMessage::SizeLock, locale)
        );
    }

    Model& model_;
    tsl::elm::ListItem* hud_item_{};
    tsl::elm::ListItem* language_item_{};
    tsl::elm::ListItem* preset_item_{};
    tsl::elm::ToggleListItem* lock_item_{};
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
