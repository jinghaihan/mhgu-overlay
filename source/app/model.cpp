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

ResourceDiagnosticInput Model::resource_diagnostic_input() const {
  const std::scoped_lock lock(mutex_);
  return resource_diagnostic_input_;
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
    return;
  }
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

void Model::enable_runtime_feature(const core::RuntimeFeature feature) {
  const std::scoped_lock lock(mutex_);
  const auto index = core::runtime_feature_index(feature);
  if (index < settings_.runtime_features.size()) {
    settings_.runtime_features[index] = true;
  }
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
  const std::scoped_lock lock(mutex_);
  const auto index = core::numeric_feature_index(feature);
  if (index < settings_.numeric_features.size()) {
    settings_.numeric_features[index].enabled = true;
  }
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

void Model::request_quest_scan() {
  quest_scan_requested_ = true;
}

void Model::adjust_resource_diagnostic_value(
  const bool points, const int direction
) {
  constexpr std::uint32_t kMaximum = 9'999'999;
  const std::scoped_lock lock(mutex_);
  auto& value = points ? resource_diagnostic_input_.wycademy_points
                       : resource_diagnostic_input_.zenny;
  const auto delta = static_cast<std::int64_t>(
    resource_diagnostic_input_.step
  ) * direction;
  value = static_cast<std::uint32_t>(std::clamp(
    static_cast<std::int64_t>(value) + delta,
    std::int64_t{0},
    static_cast<std::int64_t>(kMaximum)
  ));
}

void Model::adjust_resource_diagnostic_step(const int direction) {
  const std::scoped_lock lock(mutex_);
  auto& step = resource_diagnostic_input_.step;
  if (direction < 0) {
    step = std::max<std::uint32_t>(1, step / 10);
  } else if (direction > 0) {
    step = std::min<std::uint32_t>(1'000'000, step * 10);
  }
}

void Model::request_resource_scan(const bool filter) {
  constexpr auto kPending = std::uint64_t{1} << 63;
  constexpr auto kFilter = std::uint64_t{1} << 48;
  const auto input = resource_diagnostic_input();
  const auto request =
    kPending | (filter ? kFilter : 0) |
    (static_cast<std::uint64_t>(input.wycademy_points) << 24) |
    input.zenny;
  resource_scan_request_ = request;
}

void Model::persist(const core::CoreSettings& settings) {
  store_.save(settings);
}

void Model::worker_main() {
  using Clock = std::chrono::steady_clock;
  constexpr auto kFullPollInterval = std::chrono::milliseconds(250);
  constexpr auto kDamagePollInterval = std::chrono::milliseconds(33);
  constexpr auto kDiagnosticScanPollInterval = std::chrono::milliseconds(16);

  platform::switch_adapter::GameSession session;
  session.initialize();
  auto next_full_poll = Clock::now();
  auto next_damage_poll = next_full_poll;
  auto next_quest_scan_poll = next_full_poll;
  auto next_resource_scan_poll = next_full_poll;
  while (running_) {
    const auto now = Clock::now();
    if (rescan_requested_.exchange(false)) {
      session.request_rescan();
      next_full_poll = now;
    }
    const auto current_settings = settings();
    if (now >= next_full_poll) {
      session.poll(current_settings);
      next_full_poll = Clock::now() + kFullPollInterval;
    }
    if (quest_scan_requested_.load() && session.request_quest_scan()) {
      quest_scan_requested_ = false;
      next_quest_scan_poll = now;
    }
    if (session.quest_scan_active() && now >= next_quest_scan_poll) {
      session.poll_quest_scan();
      next_quest_scan_poll = Clock::now() + kDiagnosticScanPollInterval;
    }
    constexpr auto kResourceValueMask = (std::uint64_t{1} << 24) - 1;
    constexpr auto kResourceFilter = std::uint64_t{1} << 48;
    const auto resource_scan_request = resource_scan_request_.load();
    if (resource_scan_request != 0 && session.request_resource_scan(
                                        static_cast<std::uint32_t>(
                                          resource_scan_request &
                                          kResourceValueMask
                                        ),
                                        static_cast<std::uint32_t>(
                                          resource_scan_request >> 24 &
                                          kResourceValueMask
                                        ),
                                        (resource_scan_request &
                                         kResourceFilter) != 0
                                      )) {
      resource_scan_request_ = 0;
      next_resource_scan_poll = now;
    }
    if (session.resource_scan_active() && now >= next_resource_scan_poll) {
      session.poll_resource_scan();
      next_resource_scan_poll = Clock::now() + kDiagnosticScanPollInterval;
    }
    const auto damage_now = Clock::now();
    if (current_settings.damage_display_enabled &&
        damage_now >= next_damage_poll) {
      const auto now_ms = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
          damage_now.time_since_epoch()
        ).count()
      );
      session.poll_damage(true, now_ms);
      next_damage_poll = Clock::now() + kDamagePollInterval;
    } else if (!current_settings.damage_display_enabled) {
      session.poll_damage(false, 0);
      next_damage_poll = now;
    }
    const auto item_pouch_write_request =
      item_pouch_write_request_.exchange(0);
    if (item_pouch_write_request != 0) {
      session.apply_item_pouch_quantity(
        static_cast<std::uint8_t>(item_pouch_write_request >> 8),
        static_cast<std::uint8_t>(item_pouch_write_request & 0xFF)
      );
    }
    {
      const std::scoped_lock lock(mutex_);
      view_ = session.view();
    }

    auto next_wake = next_full_poll;
    if (current_settings.damage_display_enabled) {
      next_wake = std::min(next_wake, next_damage_poll);
    }
    if (session.quest_scan_active()) {
      next_wake = std::min(next_wake, next_quest_scan_poll);
    }
    if (session.resource_scan_active()) {
      next_wake = std::min(next_wake, next_resource_scan_poll);
    }
    std::this_thread::sleep_until(next_wake);
  }
  session.shutdown();
}

}  // namespace mhgu::app
