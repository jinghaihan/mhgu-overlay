#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include "mhgu/core/damage.hpp"

namespace {

mhgu::core::HealthSnapshot snapshot(
  const std::uint32_t first_hp,
  const std::uint32_t second_hp = 0,
  const bool include_second = false
) {
  mhgu::core::HealthSnapshot result{};
  result.monsters[result.monster_count++] = {0x1000, 1, first_hp, 5000};
  if (include_second) {
    result.monsters[result.monster_count++] = {0x2000, 2, second_hp, 3000};
  }
  return result;
}

}  // namespace

int main() {
  using namespace mhgu::core;

  DamageTracker tracker;
  auto output = tracker.update(snapshot(5000), 1000);
  assert(output.event_count == 0);

  output = tracker.update(snapshot(4868), 1016);
  assert(output.event_count == 1);
  assert(output.events[0].handle == 0x1000);
  assert(output.events[0].monster_id == 1);
  assert(output.events[0].damage == 132);
  assert(output.events[0].created_at_ms == 1016);
  assert(output.events[0].sequence == 1);

  output = tracker.update(snapshot(4868), 1032);
  assert(output.event_count == 1);

  output = tracker.update(snapshot(4900), 1048);
  assert(output.event_count == 1);
  output = tracker.update(snapshot(4850), 1064);
  assert(output.event_count == 2);
  assert(output.events[1].damage == 50);

  output = tracker.update(snapshot(4800, 3000, true), 1080);
  assert(output.event_count == 3);
  assert(output.events[2].damage == 50);
  output = tracker.update(snapshot(4700, 2800, true), 1096);
  assert(output.event_count == 5);
  assert(output.events[3].damage == 100);
  assert(output.events[4].damage == 200);

  output = tracker.update(snapshot(0, 2800, true), 1112);
  assert(output.event_count == 6);
  assert(output.events[5].damage == 4700);

  auto reused = snapshot(4000, 2800, true);
  reused.monsters[0].monster_id = 3;
  reused.monsters[0].max_hp = 4000;
  output = tracker.update(reused, 1128);
  assert(output.event_count == 6);

  HealthSnapshot empty{};
  output = tracker.update(empty, 1144);
  assert(output.event_count == 6);
  output = tracker.update(reused, 1160);
  assert(output.event_count == 6);

  output = tracker.current(1016 + kDamageEventLifetimeMs);
  assert(output.event_count == 5);
  assert(output.events[0].sequence == 2);

  tracker.reset();
  output = tracker.current(2000);
  assert(output.event_count == 0);
  output = tracker.update(snapshot(5000), 2000);
  assert(output.event_count == 0);

  for (std::size_t index = 0; index < kMaxDamageEvents + 2; ++index) {
    output = tracker.update(snapshot(4900), 2001 + index * 2);
    output = tracker.update(snapshot(5000), 2002 + index * 2);
  }
  assert(output.event_count == kMaxDamageEvents);
  assert(output.events[0].sequence == 3);
  assert(output.events[kMaxDamageEvents - 1].sequence == kMaxDamageEvents + 2);

  HealthSnapshot invalid{};
  invalid.monster_count = 3;
  invalid.monsters[0] = {0, 1, 100, 100};
  invalid.monsters[1] = {0x3000, 0, 100, 100};
  invalid.monsters[2] = {0x4000, 4, 101, 100};
  output = tracker.update(invalid, 2100);
  assert(output.event_count == kMaxDamageEvents);

  std::cout << "damage tests passed\n";
}
