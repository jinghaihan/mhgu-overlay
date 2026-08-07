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
      static_cast<int>(setting.value) + delta,
      static_cast<int>(range.minimum),
      static_cast<int>(range.maximum)
    );
    if (adjusted == setting.value) {
      return;
    }
    setting.value = static_cast<std::uint16_t>(adjusted);
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
  platform::switch_adapter::GameSession session;
  session.initialize();
  while (running_) {
    if (rescan_requested_.exchange(false)) {
      session.request_rescan();
    }
    const auto current_settings = settings();
    session.poll(current_settings);
    {
      const std::scoped_lock lock(mutex_);
      view_ = session.view();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
  }
  session.shutdown();
}

}  // namespace mhgu::app
