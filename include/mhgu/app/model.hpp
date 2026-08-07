#pragma once

#include <atomic>
#include <mutex>
#include <thread>

#include "mhgu/app/settings.hpp"
#include "mhgu/platform/switch/game_session.hpp"

namespace mhgu::app {

class Model {
public:
  Model();
  ~Model();

  void start();
  void stop();

  core::CoreSettings settings() const;
  platform::switch_adapter::SessionView session_view() const;
  core::Locale display_locale() const;

  void cycle_language();
  void cycle_frame_rate();
  void enable_map_and_large_monsters();
  void cycle_size_preset();
  void request_rescan();

private:
  void worker_main();
  void persist(const core::CoreSettings& settings);

  mutable std::mutex mutex_;
  core::CoreSettings settings_{};
  platform::switch_adapter::SessionView view_{};
  SettingsStore store_;
  std::atomic<bool> running_{false};
  std::atomic<bool> rescan_requested_{false};
  std::thread worker_;
};

}  // namespace mhgu::app
