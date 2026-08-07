#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "mhgu/core/types.hpp"

namespace mhgu::core {

constexpr std::size_t kMaxDamageEvents = 16;
constexpr std::uint64_t kDamageEventLifetimeMs = 900;

struct MonsterHealthSample {
  MonsterHandle handle;
  MonsterId monster_id;
  std::uint32_t hp;
  std::uint32_t max_hp;
};

struct HealthSnapshot {
  std::array<MonsterHealthSample, kMaxMonsters> monsters{};
  std::size_t monster_count{};
};

struct DamageEvent {
  MonsterHandle handle;
  MonsterId monster_id;
  std::uint32_t damage;
  std::uint64_t created_at_ms;
  std::uint64_t sequence;
};

struct DamageOutput {
  std::array<DamageEvent, kMaxDamageEvents> events{};
  std::size_t event_count{};
};

class DamageTracker {
public:
  DamageOutput update(const HealthSnapshot& snapshot, std::uint64_t now_ms);
  DamageOutput current(std::uint64_t now_ms);
  void reset();

private:
  struct TrackedMonster {
    MonsterHandle handle;
    MonsterId monster_id;
    std::uint32_t hp;
    std::uint32_t max_hp;
  };

  void append_event(
    const MonsterHealthSample& sample,
    std::uint32_t damage,
    std::uint64_t now_ms
  );
  void prune_events(std::uint64_t now_ms);
  DamageOutput output() const;

  std::array<TrackedMonster, kMaxMonsters> tracked_{};
  std::size_t tracked_count_{};
  std::array<DamageEvent, kMaxDamageEvents> events_{};
  std::size_t event_count_{};
  std::uint64_t next_sequence_{1};
};

}  // namespace mhgu::core
