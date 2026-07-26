#include <cassert>
#include <cstdio>
#include <iostream>

#include "mhgu/app/settings.hpp"

int main() {
    using namespace mhgu;

    constexpr const char* kPath = "/tmp/mhgu-overlay-settings-tests.ini";
    std::remove(kPath);

    app::SettingsStore store(kPath);
    const auto defaults = store.load();
    assert(defaults.locale_mode == core::LocaleMode::Auto);
    assert(defaults.size_preset == core::SizePreset::Off);
    assert(!defaults.size_lock_armed);

    core::CoreSettings expected{};
    expected.locale_mode = core::LocaleMode::SimplifiedChinese;
    expected.size_preset = core::SizePreset::Gold;
    expected.size_lock_armed = true;
    assert(store.save(expected));

    const auto restored = store.load();
    assert(restored.locale_mode == expected.locale_mode);
    assert(restored.size_preset == expected.size_preset);
    assert(restored.size_lock_armed == expected.size_lock_armed);

    std::remove(kPath);
    std::cout << "settings tests passed\n";
}
