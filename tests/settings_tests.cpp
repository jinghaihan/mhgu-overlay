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
  assert(defaults.frame_rate == core::FrameRate::Fps30);
  assert(!defaults.show_map_and_large_monsters);

  core::CoreSettings expected{};
  expected.locale_mode = core::LocaleMode::SimplifiedChinese;
  expected.size_preset = core::SizePreset::Gold;
  expected.frame_rate = core::FrameRate::Fps60;
  expected.show_map_and_large_monsters = true;
  assert(store.save(expected));

  const auto restored = store.load();
  assert(restored.locale_mode == expected.locale_mode);
  assert(restored.size_preset == expected.size_preset);
  assert(restored.frame_rate == expected.frame_rate);
  assert(!restored.show_map_and_large_monsters);

  auto* legacy = std::fopen(kPath, "w");
  assert(legacy != nullptr);
  std::fprintf(legacy, "size_preset=gold\nsize_lock=0\n");
  assert(std::fclose(legacy) == 0);
  assert(store.load().size_preset == core::SizePreset::Off);

  legacy = std::fopen(kPath, "w");
  assert(legacy != nullptr);
  std::fprintf(legacy, "size_preset=silver\nsize_lock=1\n");
  assert(std::fclose(legacy) == 0);
  assert(store.load().size_preset == core::SizePreset::Silver);

  std::remove(kPath);
  std::cout << "settings tests passed\n";
}
