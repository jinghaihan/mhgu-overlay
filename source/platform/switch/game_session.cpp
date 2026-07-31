#include "mhgu/platform/switch/game_session.hpp"

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
      heap_base_ != metadata.heap_extents.base ||
      heap_size_ != metadata.heap_extents.size) {
    profile_ = next_profile;
    process_id_ = metadata.process_id;
    heap_base_ = metadata.heap_extents.base;
    heap_size_ = metadata.heap_extents.size;
    reader_ = std::make_unique<MonsterReader>(
      memory_, *profile_, heap_base_, heap_size_
    );
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
  reader_.reset();
  profile_ = nullptr;
  process_id_ = 0;
  heap_base_ = 0;
  heap_size_ = 0;
  view_ = {};
  view_.status = status;
}

void GameSession::poll(const core::CoreSettings& settings) {
  if (!initialized_ && !initialize()) {
    detach(SessionStatus::NoGame);
    return;
  }
  if (!attach() || reader_ == nullptr) {
    return;
  }

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

void GameSession::request_rescan() {
  view_.pointer_list = 0;
  view_.output = {};
  if (profile_ != nullptr) {
    view_.status = SessionStatus::Searching;
  }
}

const SessionView& GameSession::view() const {
  return view_;
}

}  // namespace mhgu::platform::switch_adapter
