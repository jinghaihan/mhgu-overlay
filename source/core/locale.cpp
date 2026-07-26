#include "mhgu/core/locale.hpp"

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
    switch (locale) {
        case Locale::SimplifiedChinese:
            switch (crown) {
                case Crown::Mini: return "小金";
                case Crown::Silver: return "大银";
                case Crown::Gold: return "大金";
                default: return "";
            }
        case Locale::Japanese:
            switch (crown) {
                case Crown::Mini: return "小冠";
                case Crown::Silver: return "銀冠";
                case Crown::Gold: return "金冠";
                default: return "";
            }
        case Locale::English:
        default:
            switch (crown) {
                case Crown::Mini: return "MINI";
                case Crown::Silver: return "SILVER";
                case Crown::Gold: return "GOLD";
                default: return "";
            }
    }
}

const char* hyper_label(const Locale locale) {
    switch (locale) {
        case Locale::SimplifiedChinese: return "狞猛";
        case Locale::Japanese: return "獰猛";
        case Locale::English:
        default: return "Hyper";
    }
}

const char* size_preset_label(
    const SizePreset preset,
    const Locale locale
) {
    if (preset == SizePreset::Off) {
        switch (locale) {
            case Locale::SimplifiedChinese: return "关闭";
            case Locale::Japanese: return "オフ";
            default: return "Off";
        }
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
