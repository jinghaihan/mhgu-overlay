#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

#include "mhgu/core/catalog.hpp"
#include "mhgu/platform/switch/game_patches.hpp"
#include "mhgu/platform/switch/game_profile.hpp"
#include "mhgu/platform/switch/language.hpp"
#include "mhgu/platform/switch/memory.hpp"
#include "mhgu/platform/switch/monster_reader.hpp"

namespace {

class FakeMemory final : public mhgu::platform::switch_adapter::MemoryAccess {
public:
  explicit FakeMemory(const std::size_t size)
    : bytes_(size) {}

  bool read(
    const std::uint64_t address, void* destination, const std::size_t size
  ) override {
    if (address > bytes_.size() || size > bytes_.size() - address) {
      return false;
    }
    std::memcpy(destination, bytes_.data() + address, size);
    return true;
  }

  bool write(
    const std::uint64_t address, const void* source, const std::size_t size
  ) override {
    if (address > bytes_.size() || size > bytes_.size() - address) {
      return false;
    }
    std::memcpy(bytes_.data() + address, source, size);
    ++write_count_;
    return true;
  }

  template <typename T> void store(const std::size_t address, const T& value) {
    assert(write(address, &value, sizeof(value)));
  }

  template <typename T> T load(const std::size_t address) {
    T value{};
    assert(read(address, &value, sizeof(value)));
    return value;
  }

  std::size_t write_count() const {
    return write_count_;
  }

private:
  std::vector<std::uint8_t> bytes_;
  std::size_t write_count_{};
};

}  // namespace

int main() {
  using namespace mhgu;
  using namespace platform::switch_adapter;

  std::array<std::uint8_t, 0x20> build_id{};
  std::copy(kMhgu140BuildId.begin(), kMhgu140BuildId.end(), build_id.begin());
  const auto* matched_profile =
    profile_for_process(kMhguTitleId, build_id.data(), build_id.size());
  assert(matched_profile != nullptr);
  assert(
    profile_for_process(kMhxxTitleId, build_id.data(), build_id.size()) ==
    nullptr
  );
  build_id[0] ^= 0xFFU;
  assert(
    profile_for_process(kMhguTitleId, build_id.data(), build_id.size()) ==
    nullptr
  );
  build_id[0] ^= 0xFFU;

  auto profile = *matched_profile;
  profile.scan_start_from_heap = 0x100;
  profile.scan_end_from_heap = 0x1000;

  constexpr std::uint64_t kMainBase = 0x1000;
  constexpr std::uint64_t kFrameRatePointer = 0x80;
  constexpr std::uint64_t kFrameRateTargetBase = 0x18000;
  constexpr std::uint64_t kFrameRateTargetOffset = 0x20;
  profile.frame_rate.pointer_from_main = kFrameRatePointer;
  profile.frame_rate.target_from_pointer = kFrameRateTargetOffset;
  const auto map_index = core::runtime_feature_index(
    core::RuntimeFeature::MapAndLargeMonsters
  );
  const auto carry_index = core::runtime_feature_index(
    core::RuntimeFeature::CarryItemsIntoPouch
  );
  const auto invincible_index = core::runtime_feature_index(
    core::RuntimeFeature::Invincible
  );
  const auto health_index = core::runtime_feature_index(
    core::RuntimeFeature::HealthNoDecrease
  );
  const auto stamina_index = core::runtime_feature_index(
    core::RuntimeFeature::StaminaNoDecrease
  );
  const auto sharpness_index = core::runtime_feature_index(
    core::RuntimeFeature::SharpnessNoDecrease
  );
  assert(matched_profile->runtime_patches[health_index].count == 1);
  assert(
    matched_profile->runtime_patches[health_index].patches[0].offset ==
    0x002F0CFC
  );
  assert(
    matched_profile->runtime_patches[health_index].patches[0].value ==
    0xE302170F
  );
  assert(matched_profile->runtime_patches[stamina_index].count == 1);
  assert(
    matched_profile->runtime_patches[stamina_index].patches[0].offset ==
    0x002A3EC4
  );
  assert(
    matched_profile->runtime_patches[stamina_index].patches[0].value ==
    0xE3A00001
  );
  assert(matched_profile->runtime_patches[sharpness_index].count == 1);
  assert(
    matched_profile->runtime_patches[sharpness_index].patches[0].offset ==
    0x002AD3E0
  );
  assert(
    matched_profile->runtime_patches[sharpness_index].patches[0].value ==
    0xE6BF1070
  );
  profile.runtime_patches[map_index].patches[0].offset = 0x100;
  profile.runtime_patches[map_index].patches[1].offset = 0x104;
  profile.runtime_patches[carry_index].patches[0].offset = 0x108;
  profile.runtime_patches[invincible_index].patches[0].offset = 0x10C;
  profile.runtime_patches[health_index].patches[0].offset = 0x110;
  profile.runtime_patches[stamina_index].patches[0].offset = 0x114;
  profile.runtime_patches[sharpness_index].patches[0].offset = 0x118;

  constexpr std::uint64_t kList = 0x200;
  constexpr std::uint32_t kMonster = 0x4000;
  FakeMemory memory(0x20000);
  memory.store(
    kMainBase + kFrameRatePointer, kFrameRateTargetBase
  );
  memory.store(
    kFrameRateTargetBase + kFrameRateTargetOffset,
    profile.frame_rate.fps30_value
  );
  constexpr std::uint32_t kOriginalShowMapInstruction = 0x0A000001;
  constexpr std::uint32_t kOriginalMarkInstruction = 0xE3A00000;
  constexpr std::uint32_t kOriginalCarryInstruction = 0x1A000001;
  constexpr std::uint32_t kOriginalInvincibleInstruction = 0xE3500000;
  constexpr std::uint32_t kOriginalHealthInstruction = 0xE1A01000;
  constexpr std::uint32_t kOriginalStaminaInstruction = 0xE3500000;
  constexpr std::uint32_t kOriginalSharpnessInstruction = 0xE1A01000;
  memory.store(
    kMainBase + profile.runtime_patches[map_index].patches[0].offset,
    kOriginalShowMapInstruction
  );
  memory.store(
    kMainBase + profile.runtime_patches[map_index].patches[1].offset,
    kOriginalMarkInstruction
  );
  memory.store(
    kMainBase + profile.runtime_patches[carry_index].patches[0].offset,
    kOriginalCarryInstruction
  );
  memory.store(
    kMainBase + profile.runtime_patches[invincible_index].patches[0].offset,
    kOriginalInvincibleInstruction
  );
  memory.store(
    kMainBase + profile.runtime_patches[health_index].patches[0].offset,
    kOriginalHealthInstruction
  );
  memory.store(
    kMainBase + profile.runtime_patches[stamina_index].patches[0].offset,
    kOriginalStaminaInstruction
  );
  memory.store(
    kMainBase + profile.runtime_patches[sharpness_index].patches[0].offset,
    kOriginalSharpnessInstruction
  );
  GamePatches patches(
    memory, profile, kMainBase, 0x1000, 0, 0x20000
  );
  assert(!patches.enable_runtime_feature(core::RuntimeFeature::Count));
  auto patch_writes = memory.write_count();
  assert(patches.set_frame_rate(core::FrameRate::Fps60));
  assert(memory.write_count() == patch_writes + 1);
  assert(
    memory.load<std::uint32_t>(
      kFrameRateTargetBase + kFrameRateTargetOffset
    ) == profile.frame_rate.fps60_value
  );
  patch_writes = memory.write_count();
  assert(patches.set_frame_rate(core::FrameRate::Fps60));
  assert(memory.write_count() == patch_writes);
  assert(patches.set_frame_rate(core::FrameRate::Fps30));
  assert(
    memory.load<std::uint32_t>(
      kFrameRateTargetBase + kFrameRateTargetOffset
    ) == profile.frame_rate.fps30_value
  );

  constexpr std::uint32_t kUnexpectedFrameRate = 0xDEADBEEF;
  memory.store(
    kFrameRateTargetBase + kFrameRateTargetOffset, kUnexpectedFrameRate
  );
  patch_writes = memory.write_count();
  assert(!patches.set_frame_rate(core::FrameRate::Fps60));
  assert(memory.write_count() == patch_writes);
  memory.store(
    kFrameRateTargetBase + kFrameRateTargetOffset,
    profile.frame_rate.fps30_value
  );

  auto invalid_profile = profile;
  invalid_profile.frame_rate.pointer_from_main = 0x1000;
  GamePatches invalid_source(
    memory, invalid_profile, kMainBase, 0x1000, 0, 0x20000
  );
  patch_writes = memory.write_count();
  assert(!invalid_source.set_frame_rate(core::FrameRate::Fps60));
  assert(memory.write_count() == patch_writes);

  memory.store(kMainBase + kFrameRatePointer, std::uint64_t{0x1FFF0});
  patch_writes = memory.write_count();
  assert(!patches.set_frame_rate(core::FrameRate::Fps60));
  assert(memory.write_count() == patch_writes);
  memory.store(
    kMainBase + kFrameRatePointer, kFrameRateTargetBase
  );

  patch_writes = memory.write_count();
  assert(patches.enable_runtime_feature(
    core::RuntimeFeature::MapAndLargeMonsters
  ));
  assert(memory.write_count() == patch_writes + 2);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.runtime_patches[map_index].patches[0].offset
    ) == profile.runtime_patches[map_index].patches[0].value
  );
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.runtime_patches[map_index].patches[1].offset
    ) == profile.runtime_patches[map_index].patches[1].value
  );
  patch_writes = memory.write_count();
  assert(patches.enable_runtime_feature(
    core::RuntimeFeature::MapAndLargeMonsters
  ));
  assert(memory.write_count() == patch_writes);

  auto invalid_map_profile = profile;
  invalid_map_profile.runtime_patches[map_index].patches[1].offset = 0x1000;
  memory.store(
    kMainBase + profile.runtime_patches[map_index].patches[0].offset,
    kOriginalShowMapInstruction
  );
  GamePatches invalid_map(
    memory, invalid_map_profile, kMainBase, 0x1000, 0, 0x20000
  );
  patch_writes = memory.write_count();
  assert(!invalid_map.enable_runtime_feature(
    core::RuntimeFeature::MapAndLargeMonsters
  ));
  assert(memory.write_count() == patch_writes);

  patch_writes = memory.write_count();
  assert(patches.enable_runtime_feature(
    core::RuntimeFeature::CarryItemsIntoPouch
  ));
  assert(memory.write_count() == patch_writes + 1);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.runtime_patches[carry_index].patches[0].offset
    ) == profile.runtime_patches[carry_index].patches[0].value
  );
  patch_writes = memory.write_count();
  assert(patches.enable_runtime_feature(
    core::RuntimeFeature::CarryItemsIntoPouch
  ));
  assert(memory.write_count() == patch_writes);

  auto invalid_carry_profile = profile;
  invalid_carry_profile.runtime_patches[carry_index].patches[0].offset =
    0x1000;
  GamePatches invalid_carry(
    memory, invalid_carry_profile, kMainBase, 0x1000, 0, 0x20000
  );
  patch_writes = memory.write_count();
  assert(!invalid_carry.enable_runtime_feature(
    core::RuntimeFeature::CarryItemsIntoPouch
  ));
  assert(memory.write_count() == patch_writes);

  patch_writes = memory.write_count();
  assert(patches.enable_runtime_feature(core::RuntimeFeature::Invincible));
  assert(memory.write_count() == patch_writes + 1);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.runtime_patches[invincible_index].patches[0].offset
    ) == profile.runtime_patches[invincible_index].patches[0].value
  );
  patch_writes = memory.write_count();
  assert(patches.enable_runtime_feature(core::RuntimeFeature::Invincible));
  assert(memory.write_count() == patch_writes);

  auto invalid_invincible_profile = profile;
  invalid_invincible_profile.runtime_patches[invincible_index]
    .patches[0]
    .offset = 0x1000;
  GamePatches invalid_invincible(
    memory, invalid_invincible_profile, kMainBase, 0x1000, 0, 0x20000
  );
  patch_writes = memory.write_count();
  assert(!invalid_invincible.enable_runtime_feature(
    core::RuntimeFeature::Invincible
  ));
  assert(memory.write_count() == patch_writes);

  patch_writes = memory.write_count();
  assert(patches.enable_runtime_feature(
    core::RuntimeFeature::HealthNoDecrease
  ));
  assert(memory.write_count() == patch_writes + 1);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.runtime_patches[health_index].patches[0].offset
    ) == profile.runtime_patches[health_index].patches[0].value
  );

  patch_writes = memory.write_count();
  assert(patches.enable_runtime_feature(
    core::RuntimeFeature::StaminaNoDecrease
  ));
  assert(memory.write_count() == patch_writes + 1);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.runtime_patches[stamina_index].patches[0].offset
    ) == profile.runtime_patches[stamina_index].patches[0].value
  );

  patch_writes = memory.write_count();
  assert(patches.enable_runtime_feature(
    core::RuntimeFeature::SharpnessNoDecrease
  ));
  assert(memory.write_count() == patch_writes + 1);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.runtime_patches[sharpness_index].patches[0].offset
    ) == profile.runtime_patches[sharpness_index].patches[0].value
  );

  const std::uint8_t one = 1;
  memory.store(kList, one);
  memory.store(kList + 1, one);
  memory.store(kList + profile.pointer_list.pointers, kMonster);
  memory.store(kList + profile.pointer_list.count, one);

  const std::uint8_t secondary = 0x44;
  const std::uint8_t current_location = profile.monster.current_location_value;
  const std::uint16_t primary = 0x2220;
  const std::uint32_t health = 4000;
  const std::uint32_t maximum_health = 5000;
  const float size = 1.0F;
  memory.store(kMonster + profile.monster.location_flag, current_location);
  memory.store(kMonster + profile.monster.secondary_identifier, secondary);
  memory.store(kMonster + profile.monster.primary_identifier, primary);
  memory.store(kMonster + profile.monster.health, health);
  memory.store(kMonster + profile.monster.maximum_health, maximum_health);
  memory.store(kMonster + profile.monster.size_multiplier, size);

  assert(normalized_raw_id(primary, secondary) == 0x222044);
  assert(normalized_raw_id(0x21C0, secondary) == 0x222044);
  assert(resolve_monster(primary, secondary).monster_id != 0);
  const auto hyper = resolve_monster(primary, 0x4C);
  assert(hyper.monster_id == resolve_monster(primary, secondary).monster_id);
  assert(hyper.hyper);
  assert(locale_from_switch_language(0) == core::Locale::Japanese);
  assert(locale_from_switch_language(6) == core::Locale::SimplifiedChinese);
  assert(locale_from_switch_language(15) == core::Locale::SimplifiedChinese);
  assert(locale_from_switch_language(7) == core::Locale::English);
  assert(locale_from_switch_language(2) == core::Locale::English);

  MonsterReader reader(memory, profile, 0, 0x18000);
  assert(reader.find_pointer_list() == kList);
  assert(reader.validate_pointer_list(kList));

  core::GameSnapshot snapshot{};
  assert(reader.read_snapshot(kList, core::Locale::English, snapshot));
  assert(snapshot.game == core::GameId::Mhgu);
  assert(snapshot.monster_count == 1);
  assert(snapshot.monsters[0].hp == health);
  assert(snapshot.monsters[0].max_hp == maximum_health);
  assert(snapshot.monsters[0].size_percent == 100);

  const auto* rathian = core::find_monster_by_key("rathian");
  assert(rathian != nullptr);
  assert(snapshot.monsters[0].monster_id == rathian->id);
  core::SizeWriteRequest request{
    kMonster,
    rathian->id,
    rathian->gold_percent,
  };
  std::uint16_t verified{};
  assert(reader.apply_size(kList, request, verified));
  assert(verified == rathian->gold_percent);

  core::GameSnapshot changed{};
  assert(reader.read_snapshot(kList, core::Locale::English, changed));
  assert(changed.monsters[0].size_percent == rathian->gold_percent);

  request.target_percent = rathian->legal_max_percent + 1;
  assert(!reader.apply_size(kList, request, verified));
  core::GameSnapshot rejected{};
  assert(reader.read_snapshot(kList, core::Locale::English, rejected));
  assert(rejected.monsters[0].size_percent == rathian->gold_percent);

  constexpr std::uint64_t kOutsideHeap = 0x18000;
  memory.store(kOutsideHeap + profile.monster.location_flag, current_location);
  memory.store(kOutsideHeap + profile.monster.secondary_identifier, secondary);
  memory.store(kOutsideHeap + profile.monster.primary_identifier, primary);
  memory.store(kOutsideHeap + profile.monster.size_multiplier, size);
  request.handle = kOutsideHeap;
  request.target_percent = rathian->mini_percent;
  const auto writes_before_rejection = memory.write_count();
  assert(!reader.apply_size(kList, request, verified));
  assert(memory.write_count() == writes_before_rejection);
  request.handle = kMonster;

  const auto remote_location = profile.monster.remote_location_value;
  memory.store(kMonster + profile.monster.location_flag, remote_location);
  core::GameSnapshot remote{};
  assert(reader.read_snapshot(kList, core::Locale::English, remote));
  assert(remote.monster_count == 1);
  request.target_percent = rathian->mini_percent;
  assert(reader.apply_size(kList, request, verified));
  assert(verified == rathian->mini_percent);

  memory.store(kMonster + profile.monster.size_multiplier, size);
  assert(reader.apply_size(kList, request, verified));
  core::GameSnapshot reapplied{};
  assert(reader.read_snapshot(kList, core::Locale::English, reapplied));
  assert(reapplied.monsters[0].size_percent == rathian->mini_percent);

  const std::uint8_t unknown_location = 0x55;
  memory.store(kMonster + profile.monster.location_flag, unknown_location);
  core::GameSnapshot unknown{};
  assert(reader.read_snapshot(kList, core::Locale::English, unknown));
  assert(unknown.monster_count == 0);
  auto writes_before_guard = memory.write_count();
  assert(!reader.apply_size(kList, request, verified));
  assert(memory.write_count() == writes_before_guard);

  const std::uint32_t no_health = 0;
  memory.store(kMonster + profile.monster.location_flag, remote_location);
  memory.store(kMonster + profile.monster.health, no_health);
  core::GameSnapshot defeated{};
  assert(reader.read_snapshot(kList, core::Locale::English, defeated));
  assert(defeated.monster_count == 0);
  writes_before_guard = memory.write_count();
  assert(!reader.apply_size(kList, request, verified));
  assert(memory.write_count() == writes_before_guard);
  memory.store(kMonster + profile.monster.health, health);

  constexpr std::uint32_t kSecondMonster = 0xC000;
  memory.store(kSecondMonster + profile.monster.location_flag, remote_location);
  memory.store(
    kSecondMonster + profile.monster.secondary_identifier, secondary
  );
  memory.store(kSecondMonster + profile.monster.primary_identifier, primary);
  memory.store(kSecondMonster + profile.monster.health, health);
  memory.store(kSecondMonster + profile.monster.maximum_health, maximum_health);
  memory.store(kSecondMonster + profile.monster.size_multiplier, size);

  request.handle = kSecondMonster;
  writes_before_guard = memory.write_count();
  assert(!reader.apply_size(kList, request, verified));
  assert(memory.write_count() == writes_before_guard);

  const std::uint8_t two = 2;
  memory.store(
    kList + profile.pointer_list.pointers + sizeof(std::uint32_t),
    kSecondMonster
  );
  memory.store(kList + profile.pointer_list.count, two);
  memory.store(kMonster + profile.monster.location_flag, current_location);
  core::GameSnapshot multi{};
  assert(reader.read_snapshot(kList, core::Locale::English, multi));
  assert(multi.monster_count == 2);
  assert(multi.monsters[0].handle == kMonster);
  assert(multi.monsters[1].handle == kSecondMonster);

  request.target_percent = rathian->gold_percent;
  assert(reader.apply_size(kList, request, verified));
  writes_before_guard = memory.write_count();
  assert(reader.apply_size(kList, request, verified));
  assert(memory.write_count() == writes_before_guard);

  std::cout << "switch adapter tests passed\n";
}
