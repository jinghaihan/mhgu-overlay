#include "mhgu/core/locale.hpp"

#include "mhgu/core/messages.hpp"

namespace mhgu::core {

Locale resolve_locale(
    const GameId game,
    const LocaleMode mode,
    const Locale detected_locale
) {
    switch (mode) {
        case LocaleMode::English:
            return Locale::English;
        case LocaleMode::SimplifiedChinese:
            return Locale::SimplifiedChinese;
        case LocaleMode::Japanese:
            return Locale::Japanese;
        case LocaleMode::Auto:
        default:
            return game == GameId::Mhxx ? Locale::Japanese : detected_locale;
    }
}

const char* localized_name(
    const MonsterDefinition& monster,
    const Locale locale
) {
    switch (locale) {
        case Locale::SimplifiedChinese:
            return monster.names.simplified_chinese;
        case Locale::Japanese:
            return monster.names.japanese;
        case Locale::English:
        default:
            return monster.names.english;
    }
}

const char* crown_label(const Crown crown, const Locale locale) {
    switch (crown) {
        case Crown::Mini:
            return ui_message(UiMessage::CrownMini, locale);
        case Crown::Silver:
            return ui_message(UiMessage::CrownSilver, locale);
        case Crown::Gold:
            return ui_message(UiMessage::CrownGold, locale);
        default:
            return "";
    }
}

const char* hyper_label(const Locale locale) {
    return ui_message(UiMessage::Hyper, locale);
}

const char* size_preset_label(
    const SizePreset preset,
    const Locale locale
) {
    if (preset == SizePreset::Off) {
        return ui_message(UiMessage::Off, locale);
    }

    Crown crown = Crown::None;
    switch (preset) {
        case SizePreset::Mini: crown = Crown::Mini; break;
        case SizePreset::Silver: crown = Crown::Silver; break;
        case SizePreset::Gold: crown = Crown::Gold; break;
        default: break;
    }
    return crown_label(crown, locale);
}

}  // namespace mhgu::core
