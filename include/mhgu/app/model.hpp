#pragma once

#include <atomic>
#include <cstddef>
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

enum class FoodSkillApplyStatus : std::uint8_t {
  Idle,
  Pending,
  Applied,
  NoCharacterData,
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
  FoodSkillApplyStatus food_skill_apply_status() const;
  core::Locale display_locale() const;

  void cycle_language();
  void cycle_frame_rate();
  void cycle_hud_layout(int direction);
  void cycle_hud_content(int direction);
  void cycle_damage_drift_mode(int direction);
  void cycle_damage_appear_effect(int direction);
  void cycle_damage_stagger_mode(int direction);
  void cycle_damage_stagger_type(int direction);
  void toggle_damage_overlap();
  void adjust_damage_size(int delta);
  void adjust_damage_position(int delta);
  void adjust_damage_drift_distance(int delta);
  void adjust_damage_drift_speed(int delta);
  void reset_damage_display();
  void toggle_infinite_quest_time();
  void toggle_unlimited_faints();
  void request_complete_quest();
  void cycle_monster_damage_mode(int direction);
  void toggle_runtime_feature(core::RuntimeFeature feature);
  void adjust_numeric_feature(core::NumericFeature feature, int delta);
  void toggle_numeric_feature(core::NumericFeature feature);
  void adjust_item_pouch_slot(int delta);
  void adjust_item_pouch_quantity(int delta);
  void request_item_pouch_quantity_write();
  void adjust_food_skill(std::size_t slot, int delta);
  void set_food_skill(std::size_t slot, mhgu::core::FoodSkillId id);
  mhgu::core::FoodSkillId food_skill(std::size_t slot) const;
  void request_food_skills_write();
  void cycle_size_preset();
  void request_rescan();
  void set_monster_hud_active(bool active);
  bool monster_hud_active() const {
    return monster_hud_active_.load(std::memory_order_relaxed);
  }

private:
  void worker_main();
  void persist(const core::CoreSettings& settings);

  mutable std::mutex mutex_;
  core::CoreSettings settings_{};
  platform::switch_adapter::SessionView view_{};
  QuestCompletionStatus quest_completion_status_{};
  FoodSkillApplyStatus food_skill_apply_status_{};
  platform::switch_adapter::GameSession session_{};
  SettingsStore store_;
  std::atomic<bool> running_{false};
  std::atomic<bool> rescan_requested_{false};
  std::atomic<bool> monster_hud_active_{false};
  std::atomic<std::uint16_t> item_pouch_write_request_{};
  std::atomic<std::uint32_t> food_skill_write_request_{};
  std::atomic<bool> complete_quest_requested_{};
  std::thread worker_;
};

}  // namespace mhgu::app
