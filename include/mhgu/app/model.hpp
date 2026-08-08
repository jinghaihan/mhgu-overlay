#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>

#include "mhgu/app/settings.hpp"
#include "mhgu/platform/switch/game_session.hpp"

namespace mhgu::app {

enum class QuestCompletionStatus : std::uint8_t {
  Idle,
  Pending,
  Completed,
  NoActiveQuest,
  Failed,
};

class Model {
public:
  Model();
  ~Model();

  void start();
  void stop();

  core::CoreSettings settings() const;
  platform::switch_adapter::SessionView session_view() const;
  QuestCompletionStatus quest_completion_status() const;
  core::Locale display_locale() const;

  void cycle_language();
  void cycle_frame_rate();
  void toggle_damage_display();
  void toggle_infinite_quest_time();
  void toggle_unlimited_faints();
  void request_complete_quest();
  void cycle_monster_damage_mode(int direction);
  void enable_runtime_feature(core::RuntimeFeature feature);
  void adjust_numeric_feature(core::NumericFeature feature, int delta);
  void enable_numeric_feature(core::NumericFeature feature);
  void adjust_item_pouch_slot(int delta);
  void adjust_item_pouch_quantity(int delta);
  void request_item_pouch_quantity_write();
  void cycle_size_preset();
  void request_rescan();

private:
  void worker_main();
  void persist(const core::CoreSettings& settings);

  mutable std::mutex mutex_;
  core::CoreSettings settings_{};
  platform::switch_adapter::SessionView view_{};
  QuestCompletionStatus quest_completion_status_{};
  platform::switch_adapter::GameSession session_{};
  SettingsStore store_;
  std::atomic<bool> running_{false};
  std::atomic<bool> rescan_requested_{false};
  std::atomic<std::uint16_t> item_pouch_write_request_{};
  std::atomic<bool> complete_quest_requested_{};
  std::thread worker_;
};

}  // namespace mhgu::app
