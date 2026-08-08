#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>

#include "mhgu/app/settings.hpp"
#include "mhgu/platform/switch/game_session.hpp"

namespace mhgu::app {

struct ResourceDiagnosticInput {
  std::uint32_t zenny{};
  std::uint32_t wycademy_points{};
  std::uint32_t step{1};
};

class Model {
public:
  Model();
  ~Model();

  void start();
  void stop();

  core::CoreSettings settings() const;
  platform::switch_adapter::SessionView session_view() const;
  core::Locale display_locale() const;
  ResourceDiagnosticInput resource_diagnostic_input() const;

  void cycle_language();
  void cycle_frame_rate();
  void toggle_damage_display();
  void cycle_monster_damage_mode(int direction);
  void enable_runtime_feature(core::RuntimeFeature feature);
  void adjust_numeric_feature(core::NumericFeature feature, int delta);
  void enable_numeric_feature(core::NumericFeature feature);
  void adjust_item_pouch_slot(int delta);
  void adjust_item_pouch_quantity(int delta);
  void request_item_pouch_quantity_write();
  void cycle_size_preset();
  void request_rescan();
  void request_quest_scan();
  void adjust_resource_diagnostic_value(bool points, int direction);
  void adjust_resource_diagnostic_step(int direction);
  void request_resource_scan(bool filter);

private:
  void worker_main();
  void persist(const core::CoreSettings& settings);

  mutable std::mutex mutex_;
  core::CoreSettings settings_{};
  platform::switch_adapter::SessionView view_{};
  ResourceDiagnosticInput resource_diagnostic_input_{};
  platform::switch_adapter::GameSession session_{};
  SettingsStore store_;
  std::atomic<bool> running_{false};
  std::atomic<bool> rescan_requested_{false};
  std::atomic<bool> quest_scan_requested_{false};
  std::atomic<std::uint64_t> resource_scan_request_{};
  std::atomic<std::uint16_t> item_pouch_write_request_{};
  std::thread worker_;
};

}  // namespace mhgu::app
