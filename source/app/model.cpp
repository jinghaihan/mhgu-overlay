#include "mhgu/app/model.hpp"

#include <algorithm>
#include <chrono>

#include "mhgu/core/locale.hpp"

namespace mhgu::app {

Model::Model()
  : settings_{},
    view_{},
    store_("sdmc:/config/mhgu-overlay/settings.ini") {
  settings_ = store_.load();
}

Model::~Model() {
  stop();
}

void Model::start() {
  if (running_.exchange(true)) {
    return;
  }
  // libtesla invokes initServices() while a service-manager session is open.
  // Initialize Switch services synchronously here instead of racing the
  // worker thread against the end of that session.
  if (!session_.initialize()) {
    running_ = false;
    return;
  }
  worker_ = std::thread(&Model::worker_main, this);
}

void Model::stop() {
  if (!running_.exchange(false)) {
    return;
  }
  if (worker_.joinable()) {
    worker_.join();
  }
}

core::CoreSettings Model::settings() const {
  const std::scoped_lock lock(mutex_);
  return settings_;
}

platform::switch_adapter::SessionView Model::session_view() const {
  const std::scoped_lock lock(mutex_);
  return view_;
}

core::Locale Model::display_locale() const {
  const std::scoped_lock lock(mutex_);
  return core::resolve_locale(
    view_.game, settings_.locale_mode, view_.detected_locale
  );
}

void Model::cycle_language() {
  core::CoreSettings changed{};
  {
    const std::scoped_lock lock(mutex_);
    switch (settings_.locale_mode) {
      case core::LocaleMode::Auto:
        settings_.locale_mode = core::LocaleMode::English;
        break;
      case core::LocaleMode::English:
        settings_.locale_mode = core::LocaleMode::SimplifiedChinese;
        break;
      case core::LocaleMode::SimplifiedChinese:
        settings_.locale_mode = core::LocaleMode::Japanese;
        break;
      default:
        settings_.locale_mode = core::LocaleMode::Auto;
        break;
    }
    changed = settings_;
  }
  persist(changed);
}

void Model::cycle_frame_rate() {
  core::CoreSettings changed{};
  {
    const std::scoped_lock lock(mutex_);
    settings_.frame_rate =
      settings_.frame_rate == core::FrameRate::Fps30
        ? core::FrameRate::Fps60
        : core::FrameRate::Fps30;
    changed = settings_;
  }
  persist(changed);
}

void Model::toggle_damage_display() {
  core::CoreSettings changed{};
  {
    const std::scoped_lock lock(mutex_);
    settings_.damage_display_enabled = !settings_.damage_display_enabled;
    changed = settings_;
  }
  persist(changed);
}

void Model::cycle_monster_damage_mode(const int direction) {
  core::CoreSettings changed{};
  {
    const std::scoped_lock lock(mutex_);
    if (direction < 0) {
      switch (settings_.monster_damage_mode) {
        case core::MonsterDamageMode::Off:
          settings_.monster_damage_mode = core::MonsterDamageMode::LeaveOneHp;
          break;
        case core::MonsterDamageMode::InstantKill:
          settings_.monster_damage_mode = core::MonsterDamageMode::Off;
          break;
        default:
          settings_.monster_damage_mode = core::MonsterDamageMode::InstantKill;
          break;
      }
    } else {
      switch (settings_.monster_damage_mode) {
        case core::MonsterDamageMode::Off:
          settings_.monster_damage_mode = core::MonsterDamageMode::InstantKill;
          break;
        case core::MonsterDamageMode::InstantKill:
          settings_.monster_damage_mode = core::MonsterDamageMode::LeaveOneHp;
          break;
        default:
          settings_.monster_damage_mode = core::MonsterDamageMode::Off;
          break;
      }
    }
    changed = settings_;
  }
  persist(changed);
}

void Model::enable_runtime_feature(const core::RuntimeFeature feature) {
  core::CoreSettings changed{};
  {
    const std::scoped_lock lock(mutex_);
    const auto index = core::runtime_feature_index(feature);
    if (index >= settings_.runtime_features.size() ||
        settings_.runtime_features[index]) {
      return;
    }
    settings_.runtime_features[index] = true;
    changed = settings_;
  }
  persist(changed);
}

void Model::adjust_numeric_feature(
  const core::NumericFeature feature, const int delta
) {
  core::CoreSettings changed{};
  {
    const std::scoped_lock lock(mutex_);
    const auto index = core::numeric_feature_index(feature);
    if (index >= settings_.numeric_features.size()) {
      return;
    }
    auto& setting = settings_.numeric_features[index];
    const auto range = core::numeric_feature_range(feature);
    const auto adjusted = std::clamp(
      static_cast<std::int64_t>(setting.value) + delta,
      static_cast<std::int64_t>(range.minimum),
      static_cast<std::int64_t>(range.maximum)
    );
    if (adjusted == setting.value) {
      return;
    }
    setting.value = static_cast<std::uint32_t>(adjusted);
    changed = settings_;
  }
  persist(changed);
}

void Model::enable_numeric_feature(const core::NumericFeature feature) {
  core::CoreSettings changed{};
  {
    const std::scoped_lock lock(mutex_);
    const auto index = core::numeric_feature_index(feature);
    if (index >= settings_.numeric_features.size() ||
        settings_.numeric_features[index].enabled) {
      return;
    }
    settings_.numeric_features[index].enabled = true;
    changed = settings_;
  }
  persist(changed);
}

void Model::adjust_item_pouch_slot(const int delta) {
  core::CoreSettings changed{};
  {
    const std::scoped_lock lock(mutex_);
    const auto adjusted = std::clamp(
      static_cast<int>(settings_.item_pouch_slot) + delta, 1, 10
    );
    if (adjusted == settings_.item_pouch_slot) {
      return;
    }
    settings_.item_pouch_slot = static_cast<std::uint8_t>(adjusted);
    changed = settings_;
  }
  persist(changed);
}

void Model::adjust_item_pouch_quantity(const int delta) {
  core::CoreSettings changed{};
  {
    const std::scoped_lock lock(mutex_);
    const auto adjusted = std::clamp(
      static_cast<int>(settings_.item_pouch_quantity) + delta, 1, 99
    );
    if (adjusted == settings_.item_pouch_quantity) {
      return;
    }
    settings_.item_pouch_quantity = static_cast<std::uint8_t>(adjusted);
    changed = settings_;
  }
  persist(changed);
}

void Model::request_item_pouch_quantity_write() {
  const auto current_settings = settings();
  const auto request = static_cast<std::uint16_t>(
    static_cast<std::uint16_t>(current_settings.item_pouch_slot) << 8 |
    current_settings.item_pouch_quantity
  );
  item_pouch_write_request_.store(request);
}

void Model::cycle_size_preset() {
  core::CoreSettings changed{};
  {
    const std::scoped_lock lock(mutex_);
    switch (settings_.size_preset) {
      case core::SizePreset::Off:
        settings_.size_preset = core::SizePreset::Mini;
        break;
      case core::SizePreset::Mini:
        settings_.size_preset = core::SizePreset::Silver;
        break;
      case core::SizePreset::Silver:
        settings_.size_preset = core::SizePreset::Gold;
        break;
      default:
        settings_.size_preset = core::SizePreset::Off;
        break;
    }
    changed = settings_;
  }
  persist(changed);
}

void Model::request_rescan() {
  rescan_requested_ = true;
}

void Model::persist(const core::CoreSettings& settings) {
  store_.save(settings);
}

void Model::worker_main() {
  using Clock = std::chrono::steady_clock;
  constexpr auto kFullPollInterval = std::chrono::milliseconds(100);
  constexpr auto kDamagePollInterval = std::chrono::milliseconds(16);

  auto next_full_poll = Clock::now();
  auto next_damage_poll = next_full_poll;
  while (running_) {
    const auto now = Clock::now();
    if (rescan_requested_.exchange(false)) {
      session_.request_rescan();
      next_full_poll = now;
    }
    const auto current_settings = settings();
    const auto damage_now = Clock::now();
    if (current_settings.damage_display_enabled &&
        damage_now >= next_damage_poll) {
      const auto now_ms = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
          damage_now.time_since_epoch()
        ).count()
      );
      session_.poll_damage(true, now_ms);
      next_damage_poll = Clock::now() + kDamagePollInterval;
    } else if (!current_settings.damage_display_enabled) {
      session_.poll_damage(false, 0);
      next_damage_poll = now;
    }
    if (now >= next_full_poll) {
      session_.poll(current_settings);
      next_full_poll = Clock::now() + kFullPollInterval;
    }
    const auto item_pouch_write_request =
      item_pouch_write_request_.exchange(0);
    if (item_pouch_write_request != 0) {
      session_.apply_item_pouch_quantity(
        static_cast<std::uint8_t>(item_pouch_write_request >> 8),
        static_cast<std::uint8_t>(item_pouch_write_request & 0xFF)
      );
    }
    {
      const std::scoped_lock lock(mutex_);
      view_ = session_.view();
    }

    auto next_wake = next_full_poll;
    if (current_settings.damage_display_enabled) {
      next_wake = std::min(next_wake, next_damage_poll);
    }
    std::this_thread::sleep_until(next_wake);
  }
  session_.shutdown();
}

}  // namespace mhgu::app
