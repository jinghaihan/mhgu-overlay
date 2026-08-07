#include "mhgu/core/damage.hpp"

#include <algorithm>

namespace mhgu::core {
namespace {

bool valid_sample(const MonsterHealthSample& sample) {
  return sample.handle != 0 && sample.monster_id != 0 && sample.max_hp != 0 &&
         sample.hp <= sample.max_hp;
}

}  // namespace

DamageOutput DamageTracker::update(
  const HealthSnapshot& snapshot, const std::uint64_t now_ms
) {
  prune_events(now_ms);

  std::array<TrackedMonster, kMaxMonsters> next_tracked{};
  std::size_t next_tracked_count{};
  const auto sample_count = std::min(snapshot.monster_count, kMaxMonsters);
  for (std::size_t index = 0; index < sample_count; ++index) {
    const auto& sample = snapshot.monsters[index];
    if (!valid_sample(sample)) {
      continue;
    }

    const auto duplicate = std::find_if(
      next_tracked.begin(),
      next_tracked.begin() + next_tracked_count,
      [&sample](const TrackedMonster& tracked) {
        return tracked.handle == sample.handle;
      }
    );
    if (duplicate != next_tracked.begin() + next_tracked_count) {
      continue;
    }

    const auto previous = std::find_if(
      tracked_.begin(),
      tracked_.begin() + tracked_count_,
      [&sample](const TrackedMonster& tracked) {
        return tracked.handle == sample.handle &&
               tracked.monster_id == sample.monster_id &&
               tracked.max_hp == sample.max_hp;
      }
    );
    if (previous != tracked_.begin() + tracked_count_ &&
        sample.hp < previous->hp) {
      append_event(sample, previous->hp - sample.hp, now_ms);
    }

    next_tracked[next_tracked_count++] = {
      sample.handle,
      sample.monster_id,
      sample.hp,
      sample.max_hp,
    };
  }

  tracked_ = next_tracked;
  tracked_count_ = next_tracked_count;
  return output();
}

DamageOutput DamageTracker::current(const std::uint64_t now_ms) {
  prune_events(now_ms);
  return output();
}

void DamageTracker::reset() {
  tracked_ = {};
  tracked_count_ = 0;
  events_ = {};
  event_count_ = 0;
  next_sequence_ = 1;
}

void DamageTracker::append_event(
  const MonsterHealthSample& sample,
  const std::uint32_t damage,
  const std::uint64_t now_ms
) {
  if (damage == 0) {
    return;
  }
  if (event_count_ == events_.size()) {
    std::move(events_.begin() + 1, events_.end(), events_.begin());
    --event_count_;
  }
  events_[event_count_++] = {
    sample.handle,
    sample.monster_id,
    damage,
    now_ms,
    next_sequence_++,
  };
}

void DamageTracker::prune_events(const std::uint64_t now_ms) {
  const auto active_end = std::remove_if(
    events_.begin(),
    events_.begin() + event_count_,
    [now_ms](const DamageEvent& event) {
      return now_ms >= event.created_at_ms &&
             now_ms - event.created_at_ms >= kDamageEventLifetimeMs;
    }
  );
  event_count_ = static_cast<std::size_t>(active_end - events_.begin());
}

DamageOutput DamageTracker::output() const {
  DamageOutput result{};
  result.events = events_;
  result.event_count = event_count_;
  return result;
}

}  // namespace mhgu::core
