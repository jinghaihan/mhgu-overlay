#pragma once

#include <cstdint>
#include <memory>

#include "mhgu/core/engine.hpp"
#include "mhgu/platform/switch/dmnt_memory.hpp"
#include "mhgu/platform/switch/game_patches.hpp"
#include "mhgu/platform/switch/game_profile.hpp"
#include "mhgu/platform/switch/monster_reader.hpp"

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
  core::GameId game{core::GameId::Unknown};
  core::Locale detected_locale{core::Locale::English};
  core::CoreOutput output{};
  const char* profile_name{};
  std::uint64_t title_id{};
  std::uint64_t pointer_list{};
};

class GameSession {
public:
  bool initialize();
  void shutdown();
  void poll(const core::CoreSettings& settings);
  void request_rescan();

  const SessionView& view() const;

private:
  bool attach();
  void detach(SessionStatus status);
  bool sync_frame_rate(core::FrameRate frame_rate);
  bool sync_map_and_large_monsters(bool enabled);
  bool sync_carry_items_into_pouch(bool enabled);

  bool initialized_{};
  DmntMemoryAccess memory_{};
  const GameProfile* profile_{};
  std::unique_ptr<GamePatches> patches_{};
  std::unique_ptr<MonsterReader> reader_{};
  core::Engine engine_{};
  SessionView view_{};
  std::uint64_t process_id_{};
  std::uint64_t main_base_{};
  std::uint64_t main_size_{};
  std::uint64_t heap_base_{};
  std::uint64_t heap_size_{};
  std::uint64_t address_space_base_{};
  std::uint64_t address_space_size_{};
  core::FrameRate applied_frame_rate_{core::FrameRate::Fps30};
  bool map_and_large_monsters_applied_{};
  bool carry_items_into_pouch_applied_{};
};

}  // namespace mhgu::platform::switch_adapter
