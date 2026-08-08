#include "mhgu/platform/switch/game_session.hpp"

#include <algorithm>

#ifdef __SWITCH__
#include <switch.h>

#include "dmntcht.h"
#endif

#include "mhgu/platform/switch/language.hpp"

namespace mhgu::platform::switch_adapter {

bool GameSession::initialize() {
  if (initialized_) {
    return true;
  }
#ifdef __SWITCH__
  if (R_FAILED(dmntchtInitialize())) {
    return false;
  }
#endif
  initialized_ = true;
  return true;
}

void GameSession::shutdown() {
  if (!initialized_) {
    return;
  }
  detach(SessionStatus::NoGame);
#ifdef __SWITCH__
  dmntchtExit();
#endif
  initialized_ = false;
}

bool GameSession::attach() {
#ifdef __SWITCH__
  bool has_process{};
  if (R_FAILED(dmntchtHasCheatProcess(&has_process))) {
    detach(SessionStatus::NoGame);
    return false;
  }
  if (!has_process && R_FAILED(dmntchtForceOpenCheatProcess())) {
    detach(SessionStatus::NoGame);
    return false;
  }

  DmntCheatProcessMetadata metadata{};
  if (R_FAILED(dmntchtGetCheatProcessMetadata(&metadata))) {
    detach(SessionStatus::NoGame);
    return false;
  }
  const auto* next_profile = profile_for_process(
    metadata.title_id,
    metadata.main_nso_build_id,
    sizeof(metadata.main_nso_build_id)
  );
  if (next_profile == nullptr) {
    detach(SessionStatus::Unsupported);
    view_.title_id = metadata.title_id;
    return false;
  }

  if (profile_ != next_profile || process_id_ != metadata.process_id ||
      main_base_ != metadata.main_nso_extents.base ||
      main_size_ != metadata.main_nso_extents.size ||
      heap_base_ != metadata.heap_extents.base ||
      heap_size_ != metadata.heap_extents.size ||
      address_space_base_ != metadata.address_space_extents.base ||
      address_space_size_ != metadata.address_space_extents.size) {
    profile_ = next_profile;
    process_id_ = metadata.process_id;
    main_base_ = metadata.main_nso_extents.base;
    main_size_ = metadata.main_nso_extents.size;
    heap_base_ = metadata.heap_extents.base;
    heap_size_ = metadata.heap_extents.size;
    address_space_base_ = metadata.address_space_extents.base;
    address_space_size_ = metadata.address_space_extents.size;
    patches_ = std::make_unique<GamePatches>(
      memory_,
      *profile_,
      main_base_,
      main_size_,
      address_space_base_,
      address_space_size_,
      heap_base_,
      heap_size_
    );
    reader_ = std::make_unique<MonsterReader>(
      memory_, *profile_, heap_base_, heap_size_
    );
    frame_rate_applied_ = false;
    applied_frame_rate_ = core::FrameRate::Fps30;
    applied_monster_damage_mode_ = core::MonsterDamageMode::Off;
    applied_runtime_features_ = {};
    applied_numeric_features_ = {};
    damage_tracker_.reset();
    view_ = {};
    view_.status = SessionStatus::Searching;
    view_.game = profile_->game;
    view_.profile_name = profile_->name;
    view_.title_id = metadata.title_id;
    view_.detected_locale = detect_game_locale(metadata.title_id);
  }
  return true;
#else
  detach(SessionStatus::NoGame);
  return false;
#endif
}

void GameSession::detach(const SessionStatus status) {
  patches_.reset();
  reader_.reset();
  profile_ = nullptr;
  process_id_ = 0;
  main_base_ = 0;
  main_size_ = 0;
  heap_base_ = 0;
  heap_size_ = 0;
  address_space_base_ = 0;
  address_space_size_ = 0;
  frame_rate_applied_ = false;
  applied_frame_rate_ = core::FrameRate::Fps30;
  applied_monster_damage_mode_ = core::MonsterDamageMode::Off;
  applied_runtime_features_ = {};
  applied_numeric_features_ = {};
  damage_tracker_.reset();
  view_ = {};
  view_.status = status;
}

bool GameSession::sync_frame_rate(const core::FrameRate frame_rate) {
  if (frame_rate_applied_ && frame_rate == applied_frame_rate_) {
    return true;
  }
  if (patches_ == nullptr || !patches_->set_frame_rate(frame_rate)) {
    return false;
  }
  frame_rate_applied_ = true;
  applied_frame_rate_ = frame_rate;
  return true;
}

bool GameSession::sync_monster_damage_mode(
  const core::MonsterDamageMode mode
) {
  if (mode == core::MonsterDamageMode::Off ||
      mode == applied_monster_damage_mode_) {
    return true;
  }
  if (patches_ == nullptr || !patches_->set_monster_damage_mode(mode)) {
    return false;
  }
  applied_monster_damage_mode_ = mode;
  return true;
}

bool GameSession::sync_runtime_features(
  const core::CoreSettings& settings
) {
  bool success = true;
  for (std::size_t index = 0; index < core::kRuntimeFeatureCount; ++index) {
    if (!settings.runtime_features[index] ||
        applied_runtime_features_[index]) {
      continue;
    }
    if (patches_ == nullptr || !patches_->enable_runtime_feature(
                                 static_cast<core::RuntimeFeature>(index)
                               )) {
      success = false;
      continue;
    }
    applied_runtime_features_[index] = true;
  }
  return success;
}

bool GameSession::sync_numeric_features(
  const core::CoreSettings& settings
) {
  bool success = true;
  for (std::size_t index = 0; index < core::kNumericFeatureCount; ++index) {
    const auto& requested = settings.numeric_features[index];
    auto& applied = applied_numeric_features_[index];
    if (!requested.enabled ||
        (applied.enabled && applied.value == requested.value)) {
      continue;
    }
    if (patches_ == nullptr || !patches_->set_numeric_feature(
                                 static_cast<core::NumericFeature>(index),
                                 requested.value
                               )) {
      success = false;
      continue;
    }
    applied = requested;
  }
  return success;
}

void GameSession::poll(const core::CoreSettings& settings) {
  if (!initialized_ && !initialize()) {
    detach(SessionStatus::NoGame);
    return;
  }
  if (!attach() || reader_ == nullptr) {
    return;
  }
  const auto frame_rate_ok = sync_frame_rate(settings.frame_rate);
  const auto monster_damage_mode_ok =
    sync_monster_damage_mode(settings.monster_damage_mode);
  const auto runtime_features_ok = sync_runtime_features(settings);
  const auto numeric_features_ok = sync_numeric_features(settings);
  view_.patch_write_failed =
    !frame_rate_ok || !monster_damage_mode_ok || !runtime_features_ok ||
    !numeric_features_ok;

  if (view_.pointer_list == 0) {
    view_.status = SessionStatus::Searching;
    view_.pointer_list = reader_->find_pointer_list();
    if (view_.pointer_list == 0) {
      return;
    }
  }

  core::GameSnapshot snapshot{};
  if (!reader_->validate_pointer_list(view_.pointer_list) ||
      !reader_->read_snapshot(
        view_.pointer_list, view_.detected_locale, snapshot
      )) {
    view_.pointer_list = 0;
    view_.output = {};
    view_.status = SessionStatus::ReadFailed;
    return;
  }

  view_.output = engine_.update(snapshot, settings);
  view_.status = SessionStatus::Ready;
  for (std::size_t index = 0; index < view_.output.write_count; ++index) {
    std::uint16_t verified{};
    if (!reader_->apply_size(
          view_.pointer_list, view_.output.writes[index], verified
        )) {
      view_.status = SessionStatus::WriteFailed;
    }
  }
}

void GameSession::poll_damage(
  const bool enabled, const std::uint64_t now_ms
) {
  if (!enabled) {
    damage_tracker_.reset();
    view_.damage = {};
    return;
  }
  if (reader_ == nullptr) {
    view_.damage = damage_tracker_.current(now_ms);
    return;
  }

  core::HealthSnapshot snapshot{};
  const auto count = std::min(
    view_.output.monster_count, snapshot.monsters.size()
  );
  for (std::size_t index = 0; index < count; ++index) {
    const auto& monster = view_.output.monsters[index];
    std::uint32_t health{};
    if (!reader_->read_health(monster.handle, monster.max_hp, health)) {
      view_.damage = damage_tracker_.current(now_ms);
      return;
    }
    snapshot.monsters[snapshot.monster_count++] = {
      monster.handle,
      monster.monster_id,
      health,
      monster.max_hp,
    };
  }
  view_.damage = damage_tracker_.update(snapshot, now_ms);
}

void GameSession::request_rescan() {
  view_.pointer_list = 0;
  view_.output = {};
  view_.damage = {};
  damage_tracker_.reset();
  if (profile_ != nullptr) {
    view_.status = SessionStatus::Searching;
  }
}

bool GameSession::apply_item_pouch_quantity(
  const std::uint8_t slot, const std::uint8_t quantity
) {
  if (patches_ == nullptr ||
      !patches_->set_item_pouch_quantity(slot, quantity)) {
    view_.patch_write_failed = true;
    return false;
  }
  return true;
}

const SessionView& GameSession::view() const {
  return view_;
}

}  // namespace mhgu::platform::switch_adapter
