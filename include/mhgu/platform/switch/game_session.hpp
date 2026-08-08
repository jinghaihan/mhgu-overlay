#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include "mhgu/core/damage.hpp"
#include "mhgu/core/engine.hpp"
#include "mhgu/platform/switch/dmnt_memory.hpp"
#include "mhgu/platform/switch/game_patches.hpp"
#include "mhgu/platform/switch/game_profile.hpp"
#include "mhgu/platform/switch/monster_reader.hpp"
#include "mhgu/platform/switch/quest_scanner.hpp"
#include "mhgu/platform/switch/resource_scanner.hpp"

namespace mhgu::platform::switch_adapter {

enum class SessionStatus : std::uint8_t {
  NoGame,
  Unsupported,
  Searching,
  Ready,
  ReadFailed,
  WriteFailed,
};

struct SessionView {
  SessionStatus status{SessionStatus::NoGame};
  bool patch_write_failed{};
  core::GameId game{core::GameId::Unknown};
  core::Locale detected_locale{core::Locale::English};
  core::CoreOutput output{};
  core::DamageOutput damage{};
  const char* profile_name{};
  std::uint64_t title_id{};
  std::uint64_t pointer_list{};
  QuestScanView quest_scan{};
  ResourceScanView resource_scan{};
};

class GameSession {
public:
  bool initialize();
  void shutdown();
  void poll(const core::CoreSettings& settings);
  void poll_damage(bool enabled, std::uint64_t now_ms);
  void poll_quest_scan();
  void poll_resource_scan();
  void request_rescan();
  bool request_quest_scan();
  bool request_resource_scan(
    std::uint32_t zenny,
    std::uint32_t wycademy_points,
    bool filter
  );
  bool quest_scan_active() const;
  bool resource_scan_active() const;
  bool apply_item_pouch_quantity(std::uint8_t slot, std::uint8_t quantity);

  const SessionView& view() const;

private:
  bool attach();
  void detach(SessionStatus status);
  bool sync_frame_rate(core::FrameRate frame_rate);
  bool sync_monster_damage_mode(core::MonsterDamageMode mode);
  bool sync_runtime_features(const core::CoreSettings& settings);
  bool sync_numeric_features(const core::CoreSettings& settings);

  bool initialized_{};
#ifdef __SWITCH__
  bool ns_initialized_{};
  bool set_initialized_{};
#endif
  DmntMemoryAccess memory_{};
  const GameProfile* profile_{};
  std::unique_ptr<GamePatches> patches_{};
  std::unique_ptr<MonsterReader> reader_{};
  std::unique_ptr<QuestScanner> quest_scanner_{};
  std::unique_ptr<ResourceScanner> resource_scanner_{};
  core::Engine engine_{};
  core::DamageTracker damage_tracker_{};
  SessionView view_{};
  std::uint64_t process_id_{};
  std::uint64_t main_base_{};
  std::uint64_t main_size_{};
  std::uint64_t heap_base_{};
  std::uint64_t heap_size_{};
  std::uint64_t address_space_base_{};
  std::uint64_t address_space_size_{};
  bool frame_rate_applied_{};
  core::FrameRate applied_frame_rate_{core::FrameRate::Fps30};
  core::MonsterDamageMode applied_monster_damage_mode_{
    core::MonsterDamageMode::Off
  };
  std::array<bool, core::kRuntimeFeatureCount> applied_runtime_features_{};
  std::array<core::NumericFeatureSetting, core::kNumericFeatureCount>
    applied_numeric_features_{};
};

}  // namespace mhgu::platform::switch_adapter
