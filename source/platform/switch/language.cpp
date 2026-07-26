#include "mhgu/platform/switch/language.hpp"

#include "mhgu/platform/switch/game_profile.hpp"

#ifdef __SWITCH__
#include <switch.h>

#include <cstddef>
#include <memory>
#endif

namespace mhgu::platform::switch_adapter {

core::Locale locale_from_switch_language(const std::int32_t language) {
    switch (language) {
        case 0:
            return core::Locale::Japanese;
        case 6:
        case 11:
        case 15:
        case 16:
            return core::Locale::SimplifiedChinese;
        case 1:
        case 12:
        default:
            return core::Locale::English;
    }
}

core::Locale detect_game_locale(const std::uint64_t title_id) {
    if (title_id == kMhxxTitleId) {
        return core::Locale::Japanese;
    }

#ifdef __SWITCH__
    if (R_SUCCEEDED(nsInitialize())) {
        auto control = std::make_unique<NsApplicationControlData>();
        u64 actual_size{};
        NacpLanguageEntry* desired{};
        const auto control_result = nsGetApplicationControlData(
            NsApplicationControlSource_Storage,
            title_id,
            control.get(),
            sizeof(*control),
            &actual_size
        );
        if (R_SUCCEEDED(control_result) &&
            actual_size >= sizeof(NacpStruct) &&
            R_SUCCEEDED(
                nsGetApplicationDesiredLanguage(&control->nacp, &desired)
            ) &&
            desired != nullptr) {
            const auto* language_entries =
                reinterpret_cast<const NacpLanguageEntry*>(&control->nacp);
            const auto index = desired - language_entries;
            nsExit();
            if (index >= 0 && index < 16) {
                return locale_from_switch_language(
                    static_cast<std::int32_t>(index)
                );
            }
            return core::Locale::English;
        }
        nsExit();
    }

    u64 language_code{};
    SetLanguage language{};
    if (R_SUCCEEDED(setInitialize())) {
        const auto language_result = setGetSystemLanguage(&language_code);
        const auto mapping_result = R_SUCCEEDED(language_result)
            ? setMakeLanguage(language_code, &language)
            : MAKERESULT(Module_Libnx, LibnxError_BadInput);
        setExit();
        if (R_SUCCEEDED(mapping_result)) {
            return locale_from_switch_language(
                static_cast<std::int32_t>(language)
            );
        }
    }
#endif

    return core::Locale::English;
}

}  // namespace mhgu::platform::switch_adapter
