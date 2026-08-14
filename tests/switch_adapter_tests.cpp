#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
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
    if (write_failure_enabled_) {
      if (successful_writes_before_failure_ == 0) {
        write_failure_enabled_ = false;
        return false;
      }
      --successful_writes_before_failure_;
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

  void fail_write_after(const std::size_t successful_writes) {
    write_failure_enabled_ = true;
    successful_writes_before_failure_ = successful_writes;
  }

private:
  std::vector<std::uint8_t> bytes_;
  std::size_t write_count_{};
  std::size_t successful_writes_before_failure_{};
  bool write_failure_enabled_{};
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
  std::array<std::uint8_t, 0x20> mhxx_build_id{};
  std::copy(
    kMhxx151BuildId.begin(), kMhxx151BuildId.end(), mhxx_build_id.begin()
  );
  const auto* mhxx_profile = profile_for_process(
    kMhxxTitleId, mhxx_build_id.data(), mhxx_build_id.size()
  );
  assert(mhxx_profile != nullptr);
  assert(mhxx_profile->game == core::GameId::Mhxx);
  assert(mhxx_profile->frame_rate.mode_pointer_from_main == 0x00DFD9CC);
  assert(mhxx_profile->frame_rate.mode_target_from_pointer == 0x8B4);
  assert(mhxx_profile->frame_rate.mode_value == 2);
  assert(mhxx_profile->frame_rate.pointer_from_main == 0x018AD81C);
  assert(mhxx_profile->frame_rate.target_from_pointer == 0x3C);
  assert(
    mhxx_profile->runtime_patches[
      core::runtime_feature_index(core::RuntimeFeature::Invincible)
    ].patches[0].offset == 0x001661D8
  );
  assert(
    mhxx_profile->numeric_patches[
      core::numeric_feature_index(core::NumericFeature::Zenny)
    ].patches[4].offset == 0x00625374
  );
  assert(
    mhxx_profile->numeric_patches[
      core::numeric_feature_index(core::NumericFeature::WycademyPoints)
    ].patches[4].offset == 0x006253A4
  );
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
  constexpr std::uint64_t kItemPouchHeapBase = 0x10000;
  constexpr std::uint64_t kItemPouchHeapSize = 0x1000;
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
  const auto hunter_art_slots_index = core::runtime_feature_index(
    core::RuntimeFeature::UnlockHunterArtSlots
  );
  const auto unlimited_hunter_arts_index = core::runtime_feature_index(
    core::RuntimeFeature::UnlimitedHunterArts
  );
  const auto valor_index = core::runtime_feature_index(
    core::RuntimeFeature::ValorGaugeNoDecrease
  );
  const auto alchemy_index = core::runtime_feature_index(
    core::RuntimeFeature::AlchemyGaugeFull
  );
  const auto sp_status_index = core::runtime_feature_index(
    core::RuntimeFeature::SpStatusNoExpire
  );
  const auto bowgun_index = core::runtime_feature_index(
    core::RuntimeFeature::BowgunAutoReload
  );
  const auto consumable_index = core::runtime_feature_index(
    core::RuntimeFeature::ConsumableItemsNoDecrease
  );
  const auto affinity_index = core::numeric_feature_index(
    core::NumericFeature::HunterAffinity
  );
  const auto palico_affinity_index = core::numeric_feature_index(
    core::NumericFeature::PalicoAffinity
  );
  const auto sp_level_index = core::numeric_feature_index(
    core::NumericFeature::SpLevel
  );
  const auto long_sword_spirit_index = core::numeric_feature_index(
    core::NumericFeature::LongSwordSpiritGauge
  );
  const auto attack_multiplier_index = core::numeric_feature_index(
    core::NumericFeature::AttackMultiplier
  );
  const auto defense_multiplier_index = core::numeric_feature_index(
    core::NumericFeature::DefenseMultiplier
  );
  const auto movement_speed_multiplier_index = core::numeric_feature_index(
    core::NumericFeature::MovementSpeedMultiplier
  );
  const auto zenny_index = core::numeric_feature_index(
    core::NumericFeature::Zenny
  );
  const auto wycademy_points_index = core::numeric_feature_index(
    core::NumericFeature::WycademyPoints
  );
  const auto weapon_transmog_index = core::runtime_feature_index(
    core::RuntimeFeature::WeaponTransmog
  );
  const auto armor_transmog_index = core::runtime_feature_index(
    core::RuntimeFeature::ArmorTransmog
  );
  const auto palico_health_index = core::runtime_feature_index(
    core::RuntimeFeature::PalicoHealthNoDecrease
  );
  assert(matched_profile->monster_damage.offset == 0x00098BB0);
  assert(
    matched_profile->monster_damage.instant_kill_value == 0xE1A00006
  );
  assert(
    matched_profile->monster_damage.leave_one_hp_value == 0xE2860001
  );
  assert(matched_profile->quest.pointer_from_main == 0x0188AD90);
  assert(matched_profile->quest.time_from_quest == 0x001C);
  assert(matched_profile->quest.time_value == 0x44610000);
  assert(matched_profile->quest.faint_count_from_quest == 0x00C4);
  assert(
    matched_profile->quest.secondary_faint_count_from_quest == 0x166A
  );
  assert(matched_profile->quest.completion_state_from_quest == 0x00EC);
  assert(matched_profile->quest.completion_value == 0x29);
  assert(
    matched_profile->item_pouch.first_quantity_from_heap == 0x10D6920C
  );
  assert(matched_profile->item_pouch.slot_stride == 4);
  assert(matched_profile->item_pouch.slot_count == 10);
  assert(matched_profile->item_pouch.minimum_quantity == 1);
  assert(matched_profile->item_pouch.maximum_quantity == 99);
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
  assert(matched_profile->runtime_patches[hunter_art_slots_index].count == 1);
  assert(
    matched_profile->runtime_patches[hunter_art_slots_index]
        .patches[0]
        .offset == 0x002778C0
  );
  assert(
    matched_profile->runtime_patches[hunter_art_slots_index]
        .patches[0]
        .value == 0xE3A00003
  );
  assert(
    matched_profile->runtime_patches[unlimited_hunter_arts_index].count == 2
  );
  assert(
    matched_profile->runtime_patches[unlimited_hunter_arts_index]
        .patches[0]
        .offset == 0x002A26E8
  );
  assert(
    matched_profile->runtime_patches[unlimited_hunter_arts_index]
        .patches[0]
        .value == 0xE18020B3
  );
  assert(
    matched_profile->runtime_patches[unlimited_hunter_arts_index]
        .patches[1]
        .offset == 0x002A26EC
  );
  assert(
    matched_profile->runtime_patches[unlimited_hunter_arts_index]
        .patches[1]
        .value == 0xE1500000
  );
  assert(matched_profile->runtime_patches[valor_index].count == 3);
  assert(
    matched_profile->runtime_patches[valor_index].patches[0].offset ==
    0x00639C68
  );
  assert(
    matched_profile->runtime_patches[valor_index].patches[0].value ==
    0xEEBD0AC1
  );
  assert(
    matched_profile->runtime_patches[valor_index].patches[1].offset ==
    0x00639C6C
  );
  assert(
    matched_profile->runtime_patches[valor_index].patches[1].value ==
    0xED800A00
  );
  assert(
    matched_profile->runtime_patches[valor_index].patches[2].offset ==
    0x00639C70
  );
  assert(
    matched_profile->runtime_patches[valor_index].patches[2].value ==
    0xED841A18
  );
  assert(matched_profile->runtime_patches[alchemy_index].count == 2);
  assert(
    matched_profile->runtime_patches[alchemy_index].patches[0].offset ==
    0x0029E980
  );
  assert(
    matched_profile->runtime_patches[alchemy_index].patches[0].value ==
    0xE3440800
  );
  assert(
    matched_profile->runtime_patches[alchemy_index].patches[1].offset ==
    0x0029E984
  );
  assert(
    matched_profile->runtime_patches[alchemy_index].patches[1].value ==
    0xE5860000
  );
  assert(matched_profile->runtime_patches[sp_status_index].count == 1);
  assert(
    matched_profile->runtime_patches[sp_status_index].patches[0].offset ==
    0x0029E5AC
  );
  assert(
    matched_profile->runtime_patches[sp_status_index].patches[0].value ==
    0xE3A00000
  );
  assert(matched_profile->runtime_patches[bowgun_index].count == 1);
  assert(
    matched_profile->runtime_patches[bowgun_index].patches[0].offset ==
    0x002FE3A8
  );
  assert(
    matched_profile->runtime_patches[bowgun_index].patches[0].value ==
    0xE1C120B2
  );
  assert(matched_profile->runtime_patches[consumable_index].count == 1);
  assert(
    matched_profile->runtime_patches[consumable_index].patches[0].offset ==
    0x003015F8
  );
  assert(
    matched_profile->runtime_patches[consumable_index].patches[0].value ==
    0xE3A07000
  );
  assert(matched_profile->numeric_patches[affinity_index].count == 1);
  assert(
    matched_profile->numeric_patches[affinity_index].patches[0].offset ==
    0x000E400C
  );
  assert(
    matched_profile->numeric_patches[affinity_index].patches[0].base_value ==
    0xE3A00000
  );
  assert(matched_profile->numeric_patches[affinity_index].minimum == 0);
  assert(matched_profile->numeric_patches[affinity_index].maximum == 100);
  assert(
    matched_profile->numeric_patches[affinity_index].patches[0].multiplier == 2
  );
  assert(matched_profile->numeric_patches[palico_affinity_index].count == 2);
  assert(
    matched_profile->numeric_patches[palico_affinity_index]
        .patches[0]
        .offset == 0x000E5C24
  );
  assert(
    matched_profile->numeric_patches[palico_affinity_index]
        .patches[1]
        .offset == 0x000E5C38
  );
  assert(matched_profile->numeric_patches[sp_level_index].count == 1);
  assert(
    matched_profile->numeric_patches[sp_level_index].patches[0].offset ==
    0x002AC5AC
  );
  assert(
    matched_profile->numeric_patches[sp_level_index].patches[0].base_value ==
    0xE3A02000
  );
  assert(
    matched_profile->numeric_patches[sp_level_index].patches[0].addend == -1
  );
  assert(matched_profile->numeric_patches[sp_level_index].minimum == 1);
  assert(matched_profile->numeric_patches[sp_level_index].maximum == 4);
  assert(
    matched_profile->numeric_patches[long_sword_spirit_index].count == 2
  );
  assert(
    matched_profile->numeric_patches[long_sword_spirit_index]
        .patches[0]
        .offset == 0x002A31C0
  );
  assert(
    matched_profile->numeric_patches[long_sword_spirit_index]
        .patches[1]
        .offset == 0x002EC6FC
  );
  assert(
    matched_profile->numeric_patches[long_sword_spirit_index]
        .patches[1]
        .encoding == NumericWordEncoding::Fixed
  );
  assert(
    matched_profile->numeric_patches[attack_multiplier_index].count == 9
  );
  assert(
    matched_profile->numeric_patches[attack_multiplier_index]
        .patches[0]
        .offset == 0x013F1E50
  );
  assert(
    matched_profile->numeric_patches[attack_multiplier_index]
        .patches[7]
        .encoding == NumericWordEncoding::LinearWord
  );
  assert(
    matched_profile->numeric_patches[attack_multiplier_index]
        .patches[8]
        .offset == 0x000E2C38
  );
  constexpr std::array<std::uint32_t, 9> kAttackPatchValues{
    0xE1DF11B4,
    0xE0000190,
    0xE3500C7F,
    0xC3A00C7F,
    0xE1C400B0,
    0xE1A00009,
    0xE12FFF1E,
    0x00000000,
    0xEB4C3C84,
  };
  for (std::size_t index = 0; index < kAttackPatchValues.size(); ++index) {
    assert(
      matched_profile->numeric_patches[attack_multiplier_index]
        .patches[index].base_value == kAttackPatchValues[index]
    );
  }
  assert(
    matched_profile->numeric_patches[attack_multiplier_index].minimum == 1
  );
  assert(
    matched_profile->numeric_patches[attack_multiplier_index].maximum == 10
  );
  assert(
    matched_profile->numeric_patches[defense_multiplier_index].count == 9
  );
  assert(
    matched_profile->numeric_patches[defense_multiplier_index]
        .patches[0]
        .offset == 0x013F1E70
  );
  assert(
    matched_profile->numeric_patches[defense_multiplier_index]
        .patches[7]
        .encoding == NumericWordEncoding::LinearWord
  );
  assert(
    matched_profile->numeric_patches[defense_multiplier_index]
        .patches[8]
        .offset == 0x000E3680
  );
  constexpr std::array<std::uint32_t, 9> kDefensePatchValues{
    0xE1DF11B4,
    0xE0000190,
    0xE3500C7F,
    0xC3A00C7F,
    0xE1C500B0,
    0xE1A00004,
    0xE12FFF1E,
    0x00000000,
    0xEB4C39FA,
  };
  for (std::size_t index = 0; index < kDefensePatchValues.size(); ++index) {
    assert(
      matched_profile->numeric_patches[defense_multiplier_index]
        .patches[index]
        .base_value == kDefensePatchValues[index]
    );
  }
  assert(
    matched_profile->numeric_patches[defense_multiplier_index].minimum == 1
  );
  assert(
    matched_profile->numeric_patches[defense_multiplier_index].maximum == 10
  );
  assert(
    matched_profile->numeric_patches[movement_speed_multiplier_index].count ==
    5
  );
  assert(
    matched_profile->numeric_patches[movement_speed_multiplier_index]
        .patches[0]
        .offset == 0x013F1E00
  );
  assert(
    matched_profile->numeric_patches[movement_speed_multiplier_index]
        .patches[3]
        .encoding == NumericWordEncoding::FloatTenths
  );
  assert(
    matched_profile->numeric_patches[movement_speed_multiplier_index]
        .patches[4]
        .offset == 0x0029BF3C
  );
  constexpr std::array<std::uint32_t, 5> kMovementSpeedPatchValues{
    0xED9F0A01,
    0xED800A00,
    0xE12FFF1E,
    0x00000000,
    0xEB4557AF,
  };
  for (
    std::size_t index = 0;
    index < kMovementSpeedPatchValues.size();
    ++index
  ) {
    assert(
      matched_profile->numeric_patches[movement_speed_multiplier_index]
        .patches[index]
        .base_value == kMovementSpeedPatchValues[index]
    );
  }
  assert(
    matched_profile->numeric_patches[movement_speed_multiplier_index]
        .minimum == 10
  );
  assert(
    matched_profile->numeric_patches[movement_speed_multiplier_index]
        .maximum == 50
  );
  assert(matched_profile->numeric_patches[zenny_index].count == 5);
  assert(
    matched_profile->numeric_patches[zenny_index].patches[0].offset ==
    0x013F1E10
  );
  assert(
    matched_profile->numeric_patches[zenny_index].patches[0].encoding ==
    NumericWordEncoding::ArmMovwImmediate
  );
  assert(
    matched_profile->numeric_patches[zenny_index].patches[1].encoding ==
    NumericWordEncoding::ArmMovtImmediate
  );
  assert(
    matched_profile->numeric_patches[zenny_index].patches[4].offset ==
    0x0062E500
  );
  constexpr std::array<std::uint32_t, 5> kZennyPatchValues{
    0xE3003000,
    0xE3403000,
    0xE5853024,
    0xE12FFF1E,
    0xEB370E42,
  };
  for (std::size_t index = 0; index < kZennyPatchValues.size(); ++index) {
    assert(
      matched_profile->numeric_patches[zenny_index]
        .patches[index]
        .base_value == kZennyPatchValues[index]
    );
  }
  assert(matched_profile->numeric_patches[zenny_index].minimum == 0);
  assert(matched_profile->numeric_patches[zenny_index].maximum == 9999999);
  assert(
    matched_profile->numeric_patches[wycademy_points_index].count == 5
  );
  assert(
    matched_profile->numeric_patches[wycademy_points_index]
        .patches[0]
        .offset == 0x013F1E20
  );
  assert(
    matched_profile->numeric_patches[wycademy_points_index]
        .patches[0]
        .encoding == NumericWordEncoding::ArmMovwImmediate
  );
  assert(
    matched_profile->numeric_patches[wycademy_points_index]
        .patches[1]
        .encoding == NumericWordEncoding::ArmMovtImmediate
  );
  assert(
    matched_profile->numeric_patches[wycademy_points_index]
        .patches[4]
        .offset == 0x0062E530
  );
  constexpr std::array<std::uint32_t, 5> kWycademyPointsPatchValues{
    0xE3003000,
    0xE3403000,
    0xE585302C,
    0xE12FFF1E,
    0xEB370E3A,
  };
  for (
    std::size_t index = 0;
    index < kWycademyPointsPatchValues.size();
    ++index
  ) {
    assert(
      matched_profile->numeric_patches[wycademy_points_index]
        .patches[index]
        .base_value == kWycademyPointsPatchValues[index]
    );
  }
  assert(
    matched_profile->numeric_patches[wycademy_points_index].minimum == 0
  );
  assert(
    matched_profile->numeric_patches[wycademy_points_index].maximum == 9999999
  );
  assert(
    matched_profile->runtime_patches[weapon_transmog_index].count == 8
  );
  assert(
    matched_profile->runtime_patches[weapon_transmog_index].patches[0].offset ==
    0x000DAEE0
  );
  assert(
    matched_profile->runtime_patches[weapon_transmog_index].patches[7].value ==
    0xE3510002
  );
  assert(matched_profile->runtime_patches[armor_transmog_index].count == 2);
  assert(
    matched_profile->runtime_patches[armor_transmog_index].patches[0].offset ==
    0x00140E9C
  );
  assert(
    matched_profile->runtime_patches[armor_transmog_index].patches[1].value ==
    0x13866000
  );
  assert(matched_profile->runtime_patches[palico_health_index].count == 6);
  assert(
    matched_profile->runtime_patches[palico_health_index].patches[0].offset ==
    0x013F1E30
  );
  assert(
    matched_profile->runtime_patches[palico_health_index].patches[5].value ==
    0xEB46637B
  );
  profile.runtime_patches[map_index].patches[0].offset = 0x100;
  profile.runtime_patches[map_index].patches[1].offset = 0x104;
  profile.runtime_patches[carry_index].patches[0].offset = 0x108;
  profile.runtime_patches[invincible_index].patches[0].offset = 0x10C;
  profile.runtime_patches[health_index].patches[0].offset = 0x110;
  profile.runtime_patches[stamina_index].patches[0].offset = 0x114;
  profile.runtime_patches[sharpness_index].patches[0].offset = 0x118;
  profile.runtime_patches[hunter_art_slots_index].patches[0].offset = 0x11C;
  profile.runtime_patches[unlimited_hunter_arts_index].patches[0].offset =
    0x120;
  profile.runtime_patches[unlimited_hunter_arts_index].patches[1].offset =
    0x124;
  profile.runtime_patches[valor_index].patches[0].offset = 0x128;
  profile.runtime_patches[valor_index].patches[1].offset = 0x12C;
  profile.runtime_patches[valor_index].patches[2].offset = 0x130;
  profile.runtime_patches[alchemy_index].patches[0].offset = 0x134;
  profile.runtime_patches[alchemy_index].patches[1].offset = 0x138;
  profile.runtime_patches[sp_status_index].patches[0].offset = 0x13C;
  profile.runtime_patches[bowgun_index].patches[0].offset = 0x140;
  profile.runtime_patches[consumable_index].patches[0].offset = 0x144;
  profile.numeric_patches[affinity_index].patches[0].offset = 0x148;
  for (std::size_t index = 0; index < 8; ++index) {
    profile.runtime_patches[weapon_transmog_index].patches[index].offset =
      0x150 + index * sizeof(std::uint32_t);
  }
  profile.runtime_patches[armor_transmog_index].patches[0].offset = 0x170;
  profile.runtime_patches[armor_transmog_index].patches[1].offset = 0x174;
  for (std::size_t index = 0; index < 6; ++index) {
    profile.runtime_patches[palico_health_index].patches[index].offset =
      0x178 + index * sizeof(std::uint32_t);
  }
  profile.numeric_patches[palico_affinity_index].patches[0].offset = 0x190;
  profile.numeric_patches[palico_affinity_index].patches[1].offset = 0x194;
  profile.numeric_patches[sp_level_index].patches[0].offset = 0x198;
  profile.numeric_patches[long_sword_spirit_index].patches[0].offset = 0x1A0;
  profile.numeric_patches[long_sword_spirit_index].patches[1].offset = 0x1A4;
  for (std::size_t index = 0; index < 9; ++index) {
    profile.numeric_patches[attack_multiplier_index].patches[index].offset =
      0x1A8 + index * sizeof(std::uint32_t);
  }
  for (std::size_t index = 0; index < 9; ++index) {
    profile.numeric_patches[defense_multiplier_index].patches[index].offset =
      0x1CC + index * sizeof(std::uint32_t);
  }
  for (std::size_t index = 0; index < 5; ++index) {
    profile.numeric_patches[movement_speed_multiplier_index]
      .patches[index]
      .offset = 0x1F0 + index * sizeof(std::uint32_t);
  }
  for (std::size_t index = 0; index < 5; ++index) {
    profile.numeric_patches[zenny_index].patches[index].offset =
      0x204 + index * sizeof(std::uint32_t);
  }
  for (std::size_t index = 0; index < 5; ++index) {
    profile.numeric_patches[wycademy_points_index].patches[index].offset =
      0x218 + index * sizeof(std::uint32_t);
  }
  profile.monster_damage.offset = 0x230;
  profile.item_pouch.first_quantity_from_heap = 0x100;
  profile.quest.pointer_from_main = 0x90;
  profile.quest.time_from_quest = 0x10;
  profile.quest.faint_count_from_quest = 0x20;
  profile.quest.secondary_faint_count_from_quest = 0x30;
  profile.quest.completion_state_from_quest = 0x40;

  constexpr std::uint64_t kList = 0x200;
  constexpr std::uint32_t kMonster = 0x4000;
  constexpr std::uint32_t kQuest = 0x14000;
  FakeMemory memory(0x20000);
  memory.store(
    kMainBase + kFrameRatePointer, kFrameRateTargetBase
  );
  memory.store(
    kFrameRateTargetBase + kFrameRateTargetOffset,
    profile.frame_rate.fps30_value
  );

  auto mhxx_frame_profile = *mhxx_profile;
  mhxx_frame_profile.frame_rate.mode_pointer_from_main = 0x40;
  mhxx_frame_profile.frame_rate.mode_target_from_pointer = 0x10;
  mhxx_frame_profile.frame_rate.pointer_from_main = 0x48;
  mhxx_frame_profile.frame_rate.target_from_pointer = 0x20;
  constexpr std::uint64_t kMhxxModeBase = 0x10000;
  constexpr std::uint64_t kMhxxFrameRateTargetBase = 0x11000;
  FakeMemory mhxx_memory(0x20000);
  mhxx_memory.store(0x40, std::uint32_t{kMhxxModeBase});
  mhxx_memory.store(kMhxxModeBase + 0x10, std::uint8_t{0});
  mhxx_memory.store(0x48, kMhxxFrameRateTargetBase);
  mhxx_memory.store(
    kMhxxFrameRateTargetBase + 0x20,
    mhxx_frame_profile.frame_rate.fps30_value
  );
  GamePatches mhxx_frame_patches(
    mhxx_memory, mhxx_frame_profile, 0, 0x1000, 0, 0x20000
  );
  assert(mhxx_frame_patches.set_frame_rate(core::FrameRate::Fps60));
  assert(
    mhxx_memory.load<std::uint8_t>(kMhxxModeBase + 0x10) ==
    mhxx_frame_profile.frame_rate.mode_value
  );
  assert(
    mhxx_memory.load<std::uint32_t>(kMhxxFrameRateTargetBase + 0x20) ==
    mhxx_frame_profile.frame_rate.fps60_value
  );

  memory.store(
    kMainBase + profile.quest.pointer_from_main, kQuest
  );
  memory.store(
    kQuest + profile.quest.time_from_quest, std::uint32_t{0}
  );
  memory.store(
    kQuest + profile.quest.faint_count_from_quest, std::uint32_t{3}
  );
  memory.store(
    kQuest + profile.quest.secondary_faint_count_from_quest,
    std::uint16_t{3}
  );
  memory.store(
    kQuest + profile.quest.completion_state_from_quest, std::uint8_t{0}
  );
  constexpr std::uint32_t kOriginalShowMapInstruction = 0x0A000001;
  constexpr std::uint32_t kOriginalMarkInstruction = 0xE3A00000;
  constexpr std::uint32_t kOriginalCarryInstruction = 0x1A000001;
  constexpr std::uint32_t kOriginalInvincibleInstruction = 0xE3500000;
  constexpr std::uint32_t kOriginalHealthInstruction = 0xE1A01000;
  constexpr std::uint32_t kOriginalStaminaInstruction = 0xE3500000;
  constexpr std::uint32_t kOriginalSharpnessInstruction = 0xE1A01000;
  constexpr std::uint32_t kOriginalHunterArtSlotsInstruction = 0xE3A00001;
  constexpr std::uint32_t kOriginalUnlimitedHunterArtsInstruction0 =
    0xE1A00000;
  constexpr std::uint32_t kOriginalUnlimitedHunterArtsInstruction1 =
    0xE3500000;
  constexpr std::uint32_t kOriginalValorInstruction0 = 0xE1A00000;
  constexpr std::uint32_t kOriginalValorInstruction1 = 0xE3500000;
  constexpr std::uint32_t kOriginalValorInstruction2 = 0xE1A00000;
  constexpr std::uint32_t kOriginalAlchemyInstruction0 = 0xE1A00000;
  constexpr std::uint32_t kOriginalAlchemyInstruction1 = 0xE3500000;
  constexpr std::uint32_t kOriginalSpStatusInstruction = 0xE3500000;
  constexpr std::uint32_t kOriginalBowgunInstruction = 0xE1A00000;
  constexpr std::uint32_t kOriginalConsumableInstruction = 0xE3500000;
  constexpr std::uint32_t kOriginalAffinityInstruction = 0xE1A00000;
  constexpr std::uint32_t kOriginalWeaponTransmogInstruction = 0xE1A00000;
  constexpr std::uint32_t kOriginalArmorTransmogInstruction = 0xE1A00000;
  constexpr std::uint32_t kOriginalPalicoHealthInstruction = 0xE1A00000;
  constexpr std::uint32_t kOriginalPalicoAffinityInstruction = 0xE1A00000;
  constexpr std::uint32_t kOriginalSpLevelInstruction = 0xE1A00000;
  constexpr std::uint32_t kOriginalLongSwordSpiritInstruction = 0xE1A00000;
  constexpr std::uint32_t kOriginalAttackMultiplierInstruction = 0xE1A00000;
  constexpr std::uint32_t kOriginalDefenseMultiplierInstruction = 0xE1A00000;
  constexpr std::uint32_t kOriginalMovementSpeedInstruction = 0xE1A00000;
  constexpr std::uint32_t kOriginalZennyInstruction = 0xE1A00000;
  constexpr std::uint32_t kOriginalWycademyPointsInstruction = 0xE1A00000;
  constexpr std::uint32_t kOriginalMonsterDamageInstruction = 0xE0800001;
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
  memory.store(
    kMainBase +
      profile.runtime_patches[hunter_art_slots_index].patches[0].offset,
    kOriginalHunterArtSlotsInstruction
  );
  memory.store(
    kMainBase +
      profile.runtime_patches[unlimited_hunter_arts_index].patches[0].offset,
    kOriginalUnlimitedHunterArtsInstruction0
  );
  memory.store(
    kMainBase +
      profile.runtime_patches[unlimited_hunter_arts_index].patches[1].offset,
    kOriginalUnlimitedHunterArtsInstruction1
  );
  memory.store(
    kMainBase + profile.runtime_patches[valor_index].patches[0].offset,
    kOriginalValorInstruction0
  );
  memory.store(
    kMainBase + profile.runtime_patches[valor_index].patches[1].offset,
    kOriginalValorInstruction1
  );
  memory.store(
    kMainBase + profile.runtime_patches[valor_index].patches[2].offset,
    kOriginalValorInstruction2
  );
  memory.store(
    kMainBase + profile.runtime_patches[alchemy_index].patches[0].offset,
    kOriginalAlchemyInstruction0
  );
  memory.store(
    kMainBase + profile.runtime_patches[alchemy_index].patches[1].offset,
    kOriginalAlchemyInstruction1
  );
  memory.store(
    kMainBase + profile.runtime_patches[sp_status_index].patches[0].offset,
    kOriginalSpStatusInstruction
  );
  memory.store(
    kMainBase + profile.runtime_patches[bowgun_index].patches[0].offset,
    kOriginalBowgunInstruction
  );
  memory.store(
    kMainBase + profile.runtime_patches[consumable_index].patches[0].offset,
    kOriginalConsumableInstruction
  );
  memory.store(
    kMainBase + profile.numeric_patches[affinity_index].patches[0].offset,
    kOriginalAffinityInstruction
  );
  for (std::size_t index = 0; index < 8; ++index) {
    memory.store(
      kMainBase + profile.runtime_patches[weapon_transmog_index]
        .patches[index].offset,
      kOriginalWeaponTransmogInstruction
    );
  }
  for (std::size_t index = 0; index < 2; ++index) {
    memory.store(
      kMainBase + profile.runtime_patches[armor_transmog_index]
        .patches[index].offset,
      kOriginalArmorTransmogInstruction
    );
  }
  memory.store(
    kMainBase + profile.numeric_patches[sp_level_index].patches[0].offset,
    kOriginalSpLevelInstruction
  );
  for (std::size_t index = 0; index < 2; ++index) {
    memory.store(
      kMainBase + profile.numeric_patches[long_sword_spirit_index]
        .patches[index].offset,
      kOriginalLongSwordSpiritInstruction
    );
  }
  for (std::size_t index = 0; index < 9; ++index) {
    memory.store(
      kMainBase + profile.numeric_patches[attack_multiplier_index]
        .patches[index].offset,
      kOriginalAttackMultiplierInstruction
    );
  }
  for (std::size_t index = 0; index < 9; ++index) {
    memory.store(
      kMainBase + profile.numeric_patches[defense_multiplier_index]
        .patches[index].offset,
      kOriginalDefenseMultiplierInstruction
    );
  }
  for (std::size_t index = 0; index < 5; ++index) {
    memory.store(
      kMainBase + profile.numeric_patches[movement_speed_multiplier_index]
        .patches[index].offset,
      kOriginalMovementSpeedInstruction
    );
  }
  for (std::size_t index = 0; index < 5; ++index) {
    memory.store(
      kMainBase + profile.numeric_patches[zenny_index].patches[index].offset,
      kOriginalZennyInstruction
    );
  }
  for (std::size_t index = 0; index < 5; ++index) {
    memory.store(
      kMainBase + profile.numeric_patches[wycademy_points_index]
        .patches[index].offset,
      kOriginalWycademyPointsInstruction
    );
  }
  memory.store(
    kMainBase + profile.monster_damage.offset,
    kOriginalMonsterDamageInstruction
  );
  for (std::size_t index = 0; index < 6; ++index) {
    memory.store(
      kMainBase + profile.runtime_patches[palico_health_index]
        .patches[index].offset,
      kOriginalPalicoHealthInstruction
    );
  }
  for (std::size_t index = 0; index < 2; ++index) {
    memory.store(
      kMainBase + profile.numeric_patches[palico_affinity_index]
        .patches[index].offset,
      kOriginalPalicoAffinityInstruction
    );
  }
  GamePatches patches(
    memory,
    profile,
    kMainBase,
    0x1000,
    0,
    0x20000,
    kItemPouchHeapBase,
    kItemPouchHeapSize
  );
  core::CoreSettings clean_settings{};
  PatchBaseline baseline{};
  assert(patches.capture_baseline(clean_settings, baseline));
  assert(patches.set_baseline(baseline));
  const char* baseline_path = "/tmp/mhgu-overlay-patch-baseline-test.bin";
  std::remove(baseline_path);
  PatchBaselineStore baseline_store{baseline_path};
  assert(baseline_store.save(profile, baseline));
  PatchBaseline loaded_baseline{};
  assert(baseline_store.load(profile, loaded_baseline));
  assert(loaded_baseline.count == baseline.count);
  assert(
    loaded_baseline.find(profile.monster_damage.offset)->value ==
    kOriginalMonsterDamageInstruction
  );
  auto wrong_baseline_profile = profile;
  wrong_baseline_profile.build_id_prefix[0] ^= 0xFFU;
  assert(!baseline_store.load(wrong_baseline_profile, loaded_baseline));
  auto* corrupt_baseline = std::fopen(baseline_path, "ab");
  assert(corrupt_baseline != nullptr);
  assert(std::fputc(0, corrupt_baseline) != EOF);
  assert(std::fclose(corrupt_baseline) == 0);
  assert(!baseline_store.load(profile, loaded_baseline));
  std::remove(baseline_path);
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
  assert(patches.set_monster_damage_mode(core::MonsterDamageMode::Off));
  assert(memory.write_count() == patch_writes);
  assert(patches.set_monster_damage_mode(
    core::MonsterDamageMode::InstantKill
  ));
  assert(memory.write_count() == patch_writes + 1);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.monster_damage.offset
    ) == profile.monster_damage.instant_kill_value
  );
  patch_writes = memory.write_count();
  assert(patches.set_monster_damage_mode(
    core::MonsterDamageMode::InstantKill
  ));
  assert(memory.write_count() == patch_writes);
  assert(patches.set_monster_damage_mode(
    core::MonsterDamageMode::LeaveOneHp
  ));
  assert(memory.write_count() == patch_writes + 1);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.monster_damage.offset
    ) == profile.monster_damage.leave_one_hp_value
  );
  patch_writes = memory.write_count();
  assert(!patches.set_monster_damage_mode(
    static_cast<core::MonsterDamageMode>(0xFF)
  ));
  assert(memory.write_count() == patch_writes);

  auto invalid_monster_damage_profile = profile;
  invalid_monster_damage_profile.monster_damage.offset = 0x1000;
  GamePatches invalid_monster_damage(
    memory,
    invalid_monster_damage_profile,
    kMainBase,
    0x1000,
    0,
    0x20000
  );
  assert(!invalid_monster_damage.set_monster_damage_mode(
    core::MonsterDamageMode::InstantKill
  ));
  assert(memory.write_count() == patch_writes);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.monster_damage.offset
    ) == profile.monster_damage.leave_one_hp_value
  );

  const auto first_item_quantity =
    kItemPouchHeapBase + profile.item_pouch.first_quantity_from_heap;
  patch_writes = memory.write_count();
  assert(patches.set_item_pouch_quantity(1, 99));
  assert(memory.write_count() == patch_writes + 1);
  assert(memory.load<std::uint8_t>(first_item_quantity) == 99);
  patch_writes = memory.write_count();
  assert(patches.set_item_pouch_quantity(1, 99));
  assert(memory.write_count() == patch_writes);
  assert(patches.set_item_pouch_quantity(10, 42));
  assert(memory.write_count() == patch_writes + 1);
  assert(
    memory.load<std::uint8_t>(
      first_item_quantity + 9 * profile.item_pouch.slot_stride
    ) == 42
  );
  patch_writes = memory.write_count();
  assert(!patches.set_item_pouch_quantity(0, 99));
  assert(!patches.set_item_pouch_quantity(11, 99));
  assert(!patches.set_item_pouch_quantity(1, 0));
  assert(!patches.set_item_pouch_quantity(1, 100));
  assert(memory.write_count() == patch_writes);

  auto invalid_item_pouch_profile = profile;
  invalid_item_pouch_profile.item_pouch.first_quantity_from_heap =
    kItemPouchHeapSize;
  GamePatches invalid_item_pouch(
    memory,
    invalid_item_pouch_profile,
    kMainBase,
    0x1000,
    0,
    0x20000,
    kItemPouchHeapBase,
    kItemPouchHeapSize
  );
  assert(!invalid_item_pouch.set_item_pouch_quantity(1, 50));
  assert(memory.write_count() == patch_writes);

  patch_writes = memory.write_count();
  assert(
    patches.maintain_quest(false, false) == QuestOperationResult::Success
  );
  assert(memory.write_count() == patch_writes);
  assert(
    patches.maintain_quest(true, false) == QuestOperationResult::Success
  );
  assert(memory.write_count() == patch_writes + 1);
  assert(
    memory.load<std::uint32_t>(
      kQuest + profile.quest.time_from_quest
    ) == profile.quest.time_value
  );
  patch_writes = memory.write_count();
  assert(
    patches.maintain_quest(true, false) == QuestOperationResult::Success
  );
  assert(memory.write_count() == patch_writes);

  assert(
    patches.maintain_quest(false, true) == QuestOperationResult::Success
  );
  assert(memory.write_count() == patch_writes + 2);
  assert(
    memory.load<std::uint32_t>(
      kQuest + profile.quest.faint_count_from_quest
    ) == 0
  );
  assert(
    memory.load<std::uint16_t>(
      kQuest + profile.quest.secondary_faint_count_from_quest
    ) == 0
  );

  patch_writes = memory.write_count();
  assert(patches.complete_quest() == QuestOperationResult::Success);
  assert(memory.write_count() == patch_writes + 1);
  assert(
    memory.load<std::uint8_t>(
      kQuest + profile.quest.completion_state_from_quest
    ) == profile.quest.completion_value
  );

  memory.store(
    kMainBase + profile.quest.pointer_from_main, std::uint32_t{0}
  );
  patch_writes = memory.write_count();
  assert(
    patches.maintain_quest(true, true) ==
    QuestOperationResult::NoActiveQuest
  );
  assert(
    patches.complete_quest() == QuestOperationResult::NoActiveQuest
  );
  assert(memory.write_count() == patch_writes);

  memory.store(
    kMainBase + profile.quest.pointer_from_main, std::uint32_t{0x1FFF0}
  );
  patch_writes = memory.write_count();
  assert(
    patches.maintain_quest(false, true) == QuestOperationResult::Failed
  );
  assert(memory.write_count() == patch_writes);

  auto invalid_quest_profile = profile;
  invalid_quest_profile.quest.pointer_from_main = 0x1000;
  GamePatches invalid_quest(
    memory,
    invalid_quest_profile,
    kMainBase,
    0x1000,
    0,
    0x20000
  );
  assert(
    invalid_quest.maintain_quest(true, true) ==
    QuestOperationResult::Failed
  );
  assert(invalid_quest.complete_quest() == QuestOperationResult::Failed);
  assert(memory.write_count() == patch_writes);

  memory.store(
    kMainBase + profile.quest.pointer_from_main, kQuest
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

  patch_writes = memory.write_count();
  assert(patches.enable_runtime_feature(
    core::RuntimeFeature::UnlockHunterArtSlots
  ));
  assert(memory.write_count() == patch_writes + 1);
  assert(
    memory.load<std::uint32_t>(
      kMainBase +
      profile.runtime_patches[hunter_art_slots_index].patches[0].offset
    ) == profile.runtime_patches[hunter_art_slots_index].patches[0].value
  );

  patch_writes = memory.write_count();
  assert(patches.enable_runtime_feature(
    core::RuntimeFeature::UnlimitedHunterArts
  ));
  assert(memory.write_count() == patch_writes + 2);
  assert(
    memory.load<std::uint32_t>(
      kMainBase +
      profile.runtime_patches[unlimited_hunter_arts_index].patches[0].offset
    ) == profile.runtime_patches[unlimited_hunter_arts_index].patches[0].value
  );
  assert(
    memory.load<std::uint32_t>(
      kMainBase +
      profile.runtime_patches[unlimited_hunter_arts_index].patches[1].offset
    ) == profile.runtime_patches[unlimited_hunter_arts_index].patches[1].value
  );

  patch_writes = memory.write_count();
  assert(patches.enable_runtime_feature(
    core::RuntimeFeature::ValorGaugeNoDecrease
  ));
  assert(memory.write_count() == patch_writes + 3);
  for (std::size_t index = 0; index < 3; ++index) {
    assert(
      memory.load<std::uint32_t>(
        kMainBase + profile.runtime_patches[valor_index].patches[index].offset
      ) == profile.runtime_patches[valor_index].patches[index].value
    );
  }

  patch_writes = memory.write_count();
  assert(patches.enable_runtime_feature(
    core::RuntimeFeature::AlchemyGaugeFull
  ));
  assert(memory.write_count() == patch_writes + 2);
  for (std::size_t index = 0; index < 2; ++index) {
    assert(
      memory.load<std::uint32_t>(
        kMainBase + profile.runtime_patches[alchemy_index].patches[index].offset
      ) == profile.runtime_patches[alchemy_index].patches[index].value
    );
  }

  patch_writes = memory.write_count();
  assert(patches.enable_runtime_feature(
    core::RuntimeFeature::PalicoHealthNoDecrease
  ));
  assert(memory.write_count() == patch_writes + 6);
  for (std::size_t index = 0; index < 6; ++index) {
    assert(
      memory.load<std::uint32_t>(
        kMainBase + profile.runtime_patches[palico_health_index]
          .patches[index].offset
      ) == profile.runtime_patches[palico_health_index].patches[index].value
    );
  }

  patch_writes = memory.write_count();
  assert(patches.enable_runtime_feature(
    core::RuntimeFeature::SpStatusNoExpire
  ));
  assert(memory.write_count() == patch_writes + 1);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.runtime_patches[sp_status_index].patches[0].offset
    ) == profile.runtime_patches[sp_status_index].patches[0].value
  );

  patch_writes = memory.write_count();
  assert(patches.enable_runtime_feature(core::RuntimeFeature::BowgunAutoReload));
  assert(memory.write_count() == patch_writes + 1);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.runtime_patches[bowgun_index].patches[0].offset
    ) == profile.runtime_patches[bowgun_index].patches[0].value
  );

  patch_writes = memory.write_count();
  assert(patches.enable_runtime_feature(
    core::RuntimeFeature::ConsumableItemsNoDecrease
  ));
  assert(memory.write_count() == patch_writes + 1);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.runtime_patches[consumable_index].patches[0].offset
    ) == profile.runtime_patches[consumable_index].patches[0].value
  );

  patch_writes = memory.write_count();
  assert(patches.set_numeric_feature(core::NumericFeature::HunterAffinity, 0));
  assert(memory.write_count() == patch_writes + 1);
  assert(
    memory.load<std::uint32_t>(
      kMainBase +
      profile.numeric_patches[affinity_index].patches[0].offset
    ) == 0xE3A00000
  );

  patch_writes = memory.write_count();
  assert(patches.set_numeric_feature(core::NumericFeature::HunterAffinity, 0));
  assert(memory.write_count() == patch_writes);
  assert(patches.set_numeric_feature(core::NumericFeature::HunterAffinity, 37));
  assert(
    memory.load<std::uint32_t>(
      kMainBase +
      profile.numeric_patches[affinity_index].patches[0].offset
    ) == 0xE3A0004A
  );
  assert(patches.set_numeric_feature(
    core::NumericFeature::HunterAffinity, 100
  ));
  assert(
    memory.load<std::uint32_t>(
      kMainBase +
      profile.numeric_patches[affinity_index].patches[0].offset
    ) == 0xE3A000C8
  );

  patch_writes = memory.write_count();
  assert(!patches.set_numeric_feature(
    core::NumericFeature::HunterAffinity, 101
  ));
  assert(!patches.set_numeric_feature(core::NumericFeature::Count, 100));
  assert(memory.write_count() == patch_writes);

  auto invalid_affinity_profile = profile;
  invalid_affinity_profile.numeric_patches[affinity_index]
    .patches[0]
    .offset = 0x1000;
  GamePatches invalid_affinity(
    memory, invalid_affinity_profile, kMainBase, 0x1000, 0, 0x20000
  );
  assert(!invalid_affinity.set_numeric_feature(
    core::NumericFeature::HunterAffinity, 100
  ));

  patch_writes = memory.write_count();
  assert(patches.set_numeric_feature(
    core::NumericFeature::PalicoAffinity, 73
  ));
  assert(memory.write_count() == patch_writes + 2);
  for (std::size_t index = 0; index < 2; ++index) {
    assert(
      memory.load<std::uint32_t>(
        kMainBase + profile.numeric_patches[palico_affinity_index]
          .patches[index].offset
      ) == 0xE3A00092
    );
  }
  patch_writes = memory.write_count();
  assert(patches.set_numeric_feature(
    core::NumericFeature::PalicoAffinity, 73
  ));
  assert(memory.write_count() == patch_writes);
  assert(!patches.set_numeric_feature(
    core::NumericFeature::PalicoAffinity, 101
  ));

  patch_writes = memory.write_count();
  assert(patches.set_numeric_feature(core::NumericFeature::SpLevel, 4));
  assert(memory.write_count() == patch_writes + 1);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[sp_level_index].patches[0].offset
    ) == 0xE3A02003
  );
  patch_writes = memory.write_count();
  assert(patches.set_numeric_feature(core::NumericFeature::SpLevel, 4));
  assert(memory.write_count() == patch_writes);
  assert(patches.set_numeric_feature(core::NumericFeature::SpLevel, 1));
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[sp_level_index].patches[0].offset
    ) == 0xE3A02000
  );
  patch_writes = memory.write_count();
  assert(!patches.set_numeric_feature(core::NumericFeature::SpLevel, 0));
  assert(memory.write_count() == patch_writes);

  patch_writes = memory.write_count();
  assert(patches.set_numeric_feature(
    core::NumericFeature::LongSwordSpiritGauge, 77
  ));
  assert(memory.write_count() == patch_writes + 2);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[long_sword_spirit_index]
        .patches[0].offset
    ) == 0xE3A0104D
  );
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[long_sword_spirit_index]
        .patches[1].offset
    ) == 0xE3A00001
  );
  patch_writes = memory.write_count();
  assert(patches.set_numeric_feature(
    core::NumericFeature::LongSwordSpiritGauge, 77
  ));
  assert(memory.write_count() == patch_writes);
  assert(patches.set_numeric_feature(
    core::NumericFeature::LongSwordSpiritGauge, 100
  ));
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[long_sword_spirit_index]
        .patches[0].offset
    ) == 0xE3A01064
  );
  assert(!patches.set_numeric_feature(
    core::NumericFeature::LongSwordSpiritGauge, 101
  ));

  patch_writes = memory.write_count();
  assert(patches.set_numeric_feature(
    core::NumericFeature::AttackMultiplier, 2
  ));
  assert(memory.write_count() == patch_writes + 9);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[attack_multiplier_index]
        .patches[0].offset
    ) == 0xE1DF11B4
  );
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[attack_multiplier_index]
        .patches[7].offset
    ) == 2
  );
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[attack_multiplier_index]
        .patches[8].offset
    ) == 0xEB4C3C84
  );
  patch_writes = memory.write_count();
  assert(patches.set_numeric_feature(
    core::NumericFeature::AttackMultiplier, 2
  ));
  assert(memory.write_count() == patch_writes);
  assert(patches.set_numeric_feature(
    core::NumericFeature::AttackMultiplier, 5
  ));
  assert(memory.write_count() == patch_writes + 1);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[attack_multiplier_index]
        .patches[7].offset
    ) == 5
  );
  patch_writes = memory.write_count();
  assert(!patches.set_numeric_feature(
    core::NumericFeature::AttackMultiplier, 0
  ));
  assert(!patches.set_numeric_feature(
    core::NumericFeature::AttackMultiplier, 11
  ));
  assert(memory.write_count() == patch_writes);

  auto invalid_attack_profile = profile;
  invalid_attack_profile.numeric_patches[attack_multiplier_index]
    .patches[8]
    .offset = 0x1000;
  GamePatches invalid_attack(
    memory, invalid_attack_profile, kMainBase, 0x1000, 0, 0x20000
  );
  patch_writes = memory.write_count();
  assert(!invalid_attack.set_numeric_feature(
    core::NumericFeature::AttackMultiplier, 3
  ));
  assert(memory.write_count() == patch_writes);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[attack_multiplier_index]
        .patches[7].offset
    ) == 5
  );

  patch_writes = memory.write_count();
  assert(patches.set_numeric_feature(
    core::NumericFeature::DefenseMultiplier, 2
  ));
  assert(memory.write_count() == patch_writes + 9);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[defense_multiplier_index]
        .patches[0].offset
    ) == 0xE1DF11B4
  );
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[defense_multiplier_index]
        .patches[7].offset
    ) == 2
  );
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[defense_multiplier_index]
        .patches[8].offset
    ) == 0xEB4C39FA
  );
  patch_writes = memory.write_count();
  assert(patches.set_numeric_feature(
    core::NumericFeature::DefenseMultiplier, 2
  ));
  assert(memory.write_count() == patch_writes);
  assert(patches.set_numeric_feature(
    core::NumericFeature::DefenseMultiplier, 5
  ));
  assert(memory.write_count() == patch_writes + 1);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[defense_multiplier_index]
        .patches[7].offset
    ) == 5
  );
  patch_writes = memory.write_count();
  assert(!patches.set_numeric_feature(
    core::NumericFeature::DefenseMultiplier, 0
  ));
  assert(!patches.set_numeric_feature(
    core::NumericFeature::DefenseMultiplier, 11
  ));
  assert(memory.write_count() == patch_writes);

  auto invalid_defense_profile = profile;
  invalid_defense_profile.numeric_patches[defense_multiplier_index]
    .patches[8]
    .offset = 0x1000;
  GamePatches invalid_defense(
    memory, invalid_defense_profile, kMainBase, 0x1000, 0, 0x20000
  );
  patch_writes = memory.write_count();
  assert(!invalid_defense.set_numeric_feature(
    core::NumericFeature::DefenseMultiplier, 3
  ));
  assert(memory.write_count() == patch_writes);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[defense_multiplier_index]
        .patches[7].offset
    ) == 5
  );

  patch_writes = memory.write_count();
  assert(patches.set_numeric_feature(
    core::NumericFeature::MovementSpeedMultiplier, 20
  ));
  assert(memory.write_count() == patch_writes + 5);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[movement_speed_multiplier_index]
        .patches[0].offset
    ) == 0xED9F0A01
  );
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[movement_speed_multiplier_index]
        .patches[3].offset
    ) == 0x40000000
  );
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[movement_speed_multiplier_index]
        .patches[4].offset
    ) == 0xEB4557AF
  );
  patch_writes = memory.write_count();
  assert(patches.set_numeric_feature(
    core::NumericFeature::MovementSpeedMultiplier, 20
  ));
  assert(memory.write_count() == patch_writes);
  assert(patches.set_numeric_feature(
    core::NumericFeature::MovementSpeedMultiplier, 25
  ));
  assert(memory.write_count() == patch_writes + 1);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[movement_speed_multiplier_index]
        .patches[3].offset
    ) == 0x40200000
  );
  patch_writes = memory.write_count();
  assert(!patches.set_numeric_feature(
    core::NumericFeature::MovementSpeedMultiplier, 9
  ));
  assert(!patches.set_numeric_feature(
    core::NumericFeature::MovementSpeedMultiplier, 51
  ));
  assert(memory.write_count() == patch_writes);

  auto invalid_movement_speed_profile = profile;
  invalid_movement_speed_profile
    .numeric_patches[movement_speed_multiplier_index]
    .patches[4]
    .offset = 0x1000;
  GamePatches invalid_movement_speed(
    memory,
    invalid_movement_speed_profile,
    kMainBase,
    0x1000,
    0,
    0x20000
  );
  patch_writes = memory.write_count();
  assert(!invalid_movement_speed.set_numeric_feature(
    core::NumericFeature::MovementSpeedMultiplier, 30
  ));
  assert(memory.write_count() == patch_writes);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[movement_speed_multiplier_index]
        .patches[3].offset
    ) == 0x40200000
  );

  patch_writes = memory.write_count();
  assert(patches.set_numeric_feature(core::NumericFeature::Zenny, 7777777));
  assert(memory.write_count() == patch_writes + 5);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[zenny_index].patches[0].offset
    ) == 0xE30A3DF1
  );
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[zenny_index].patches[1].offset
    ) == 0xE3403076
  );
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[zenny_index].patches[4].offset
    ) == 0xEB370E42
  );
  patch_writes = memory.write_count();
  assert(patches.set_numeric_feature(core::NumericFeature::Zenny, 7777777));
  assert(memory.write_count() == patch_writes);
  assert(patches.set_numeric_feature(core::NumericFeature::Zenny, 0x123456));
  assert(memory.write_count() == patch_writes + 2);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[zenny_index].patches[0].offset
    ) == 0xE3033456
  );
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[zenny_index].patches[1].offset
    ) == 0xE3403012
  );
  patch_writes = memory.write_count();
  assert(!patches.set_numeric_feature(core::NumericFeature::Zenny, 10000000));
  assert(memory.write_count() == patch_writes);

  auto invalid_zenny_profile = profile;
  invalid_zenny_profile.numeric_patches[zenny_index].patches[4].offset =
    0x1000;
  GamePatches invalid_zenny(
    memory, invalid_zenny_profile, kMainBase, 0x1000, 0, 0x20000
  );
  patch_writes = memory.write_count();
  assert(!invalid_zenny.set_numeric_feature(core::NumericFeature::Zenny, 1));
  assert(memory.write_count() == patch_writes);

  auto invalid_zenny_encoding_profile = profile;
  invalid_zenny_encoding_profile.numeric_patches[zenny_index]
    .patches[0]
    .base_value |= 1;
  GamePatches invalid_zenny_encoding(
    memory,
    invalid_zenny_encoding_profile,
    kMainBase,
    0x1000,
    0,
    0x20000
  );
  patch_writes = memory.write_count();
  assert(!invalid_zenny_encoding.set_numeric_feature(
    core::NumericFeature::Zenny, 1
  ));
  assert(memory.write_count() == patch_writes);

  patch_writes = memory.write_count();
  assert(
    patches.set_numeric_feature(
      core::NumericFeature::WycademyPoints, 7777777
    )
  );
  assert(memory.write_count() == patch_writes + 5);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[wycademy_points_index]
        .patches[0].offset
    ) == 0xE30A3DF1
  );
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[wycademy_points_index]
        .patches[1].offset
    ) == 0xE3403076
  );
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[wycademy_points_index]
        .patches[4].offset
    ) == 0xEB370E3A
  );
  patch_writes = memory.write_count();
  assert(
    patches.set_numeric_feature(
      core::NumericFeature::WycademyPoints, 7777777
    )
  );
  assert(memory.write_count() == patch_writes);
  assert(
    patches.set_numeric_feature(
      core::NumericFeature::WycademyPoints, 0x123456
    )
  );
  assert(memory.write_count() == patch_writes + 2);
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[wycademy_points_index]
        .patches[0].offset
    ) == 0xE3033456
  );
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.numeric_patches[wycademy_points_index]
        .patches[1].offset
    ) == 0xE3403012
  );
  patch_writes = memory.write_count();
  assert(
    !patches.set_numeric_feature(
      core::NumericFeature::WycademyPoints, 10000000
    )
  );
  assert(memory.write_count() == patch_writes);

  auto invalid_wycademy_points_profile = profile;
  invalid_wycademy_points_profile
    .numeric_patches[wycademy_points_index]
    .patches[4]
    .offset = 0x1000;
  GamePatches invalid_wycademy_points(
    memory,
    invalid_wycademy_points_profile,
    kMainBase,
    0x1000,
    0,
    0x20000
  );
  patch_writes = memory.write_count();
  assert(
    !invalid_wycademy_points.set_numeric_feature(
      core::NumericFeature::WycademyPoints, 1
    )
  );
  assert(memory.write_count() == patch_writes);

  auto invalid_wycademy_points_encoding_profile = profile;
  invalid_wycademy_points_encoding_profile
    .numeric_patches[wycademy_points_index]
    .patches[0]
    .base_value |= 1;
  GamePatches invalid_wycademy_points_encoding(
    memory,
    invalid_wycademy_points_encoding_profile,
    kMainBase,
    0x1000,
    0,
    0x20000
  );
  patch_writes = memory.write_count();
  assert(
    !invalid_wycademy_points_encoding.set_numeric_feature(
      core::NumericFeature::WycademyPoints, 1
    )
  );
  assert(memory.write_count() == patch_writes);

  auto invalid_palico_affinity_profile = profile;
  invalid_palico_affinity_profile.numeric_patches[palico_affinity_index]
    .patches[1]
    .offset = 0x1000;
  GamePatches invalid_palico_affinity(
    memory,
    invalid_palico_affinity_profile,
    kMainBase,
    0x1000,
    0,
    0x20000
  );
  patch_writes = memory.write_count();
  assert(!invalid_palico_affinity.set_numeric_feature(
    core::NumericFeature::PalicoAffinity, 100
  ));
  assert(memory.write_count() == patch_writes);

  auto compound_numeric_profile = profile;
  auto& compound = compound_numeric_profile.numeric_patches[affinity_index];
  compound.count = 2;
  compound.minimum = 1;
  compound.maximum = 4;
  compound.patches[0] = {
    0x198,
    0xE3A02000,
    NumericWordEncoding::LinearImmediate,
    1,
    -1,
  };
  compound.patches[1] = {
    0x19C,
    0xE3A00001,
    NumericWordEncoding::Fixed,
    0,
    0,
  };
  FakeMemory compound_memory{0x20000};
  compound_memory.store(
    kMainBase + 0x198, std::uint32_t{0xE1A00000}
  );
  compound_memory.store(
    kMainBase + 0x19C, std::uint32_t{0xE1A00000}
  );
  GamePatches compound_numeric(
    compound_memory,
    compound_numeric_profile,
    kMainBase,
    0x1000,
    0,
    0x20000
  );
  PatchBaseline compound_baseline{};
  assert(compound_numeric.capture_baseline(
    clean_settings, compound_baseline
  ));
  assert(compound_numeric.set_baseline(compound_baseline));
  assert(compound_numeric.set_numeric_feature(
    core::NumericFeature::HunterAffinity, 4
  ));
  assert(
    compound_memory.load<std::uint32_t>(kMainBase + 0x198) == 0xE3A02003
  );
  assert(
    compound_memory.load<std::uint32_t>(kMainBase + 0x19C) == 0xE3A00001
  );

  patch_writes = memory.write_count();
  assert(patches.enable_runtime_feature(
    core::RuntimeFeature::WeaponTransmog
  ));
  assert(memory.write_count() == patch_writes + 8);
  for (std::size_t index = 0; index < 8; ++index) {
    assert(
      memory.load<std::uint32_t>(
        kMainBase + profile.runtime_patches[weapon_transmog_index]
          .patches[index].offset
      ) == profile.runtime_patches[weapon_transmog_index].patches[index].value
    );
  }

  patch_writes = memory.write_count();
  assert(patches.enable_runtime_feature(core::RuntimeFeature::ArmorTransmog));
  assert(memory.write_count() == patch_writes + 2);
  for (std::size_t index = 0; index < 2; ++index) {
    assert(
      memory.load<std::uint32_t>(
        kMainBase + profile.runtime_patches[armor_transmog_index]
          .patches[index].offset
      ) == profile.runtime_patches[armor_transmog_index].patches[index].value
    );
  }

  GamePatches patched_capture(
    memory, profile, kMainBase, 0x1000, 0, 0x20000
  );
  PatchBaseline rejected_baseline{};
  assert(!patched_capture.capture_baseline(
    clean_settings, rejected_baseline
  ));

  assert(patches.set_monster_damage_mode(core::MonsterDamageMode::Off));
  assert(
    memory.load<std::uint32_t>(
      kMainBase + profile.monster_damage.offset
    ) == kOriginalMonsterDamageInstruction
  );
  assert(patches.set_runtime_feature(
    core::RuntimeFeature::ArmorTransmog, false
  ));
  for (std::size_t index = 0; index < 2; ++index) {
    assert(
      memory.load<std::uint32_t>(
        kMainBase + profile.runtime_patches[armor_transmog_index]
          .patches[index].offset
      ) == kOriginalArmorTransmogInstruction
    );
  }
  assert(patches.disable_numeric_feature(
    core::NumericFeature::Zenny, 0x123456
  ));
  for (std::size_t index = 0; index < 5; ++index) {
    assert(
      memory.load<std::uint32_t>(
        kMainBase + profile.numeric_patches[zenny_index].patches[index].offset
      ) == kOriginalZennyInstruction
    );
  }

  assert(patches.set_runtime_feature(
    core::RuntimeFeature::WeaponTransmog, false
  ));
  assert(patches.set_runtime_feature(
    core::RuntimeFeature::WeaponTransmog, true
  ));
  constexpr std::uint32_t kConflictingInstruction = 0xDEADBEEF;
  const auto& weapon_transmog = profile.runtime_patches[weapon_transmog_index];
  memory.store(
    kMainBase + weapon_transmog.patches[0].offset,
    kConflictingInstruction
  );
  assert(!patches.set_runtime_feature(
    core::RuntimeFeature::WeaponTransmog, false
  ));
  assert(
    memory.load<std::uint32_t>(
      kMainBase + weapon_transmog.patches[1].offset
    ) == weapon_transmog.patches[1].value
  );
  memory.store(
    kMainBase + weapon_transmog.patches[0].offset,
    weapon_transmog.patches[0].value
  );
  assert(patches.set_runtime_feature(
    core::RuntimeFeature::WeaponTransmog, false
  ));

  memory.fail_write_after(1);
  assert(!patches.set_runtime_feature(
    core::RuntimeFeature::ArmorTransmog, true
  ));
  for (std::size_t index = 0; index < 2; ++index) {
    assert(
      memory.load<std::uint32_t>(
        kMainBase + profile.runtime_patches[armor_transmog_index]
          .patches[index].offset
      ) == kOriginalArmorTransmogInstruction
    );
  }

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

  std::uint32_t sampled_health{};
  assert(reader.read_health(kMonster, maximum_health, sampled_health));
  assert(sampled_health == health);
  assert(!reader.read_health(kMonster, maximum_health + 1, sampled_health));
  assert(!reader.read_health(0x18000, maximum_health, sampled_health));

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
  assert(reader.read_health(kMonster, maximum_health, sampled_health));
  assert(sampled_health == 0);
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

  constexpr std::uint32_t kSmallMonster = 0x10000;
  const std::uint8_t small_identifier = 0;
  const std::uint32_t small_health = 100;
  const std::uint32_t small_maximum_health = 200;
  memory.store(
    kSmallMonster + profile.monster.location_flag, current_location
  );
  memory.store(
    kSmallMonster + profile.monster.secondary_identifier, small_identifier
  );
  memory.store(kSmallMonster + profile.monster.health, small_health);
  memory.store(
    kSmallMonster + profile.monster.maximum_health, small_maximum_health
  );
  memory.store(kList + profile.pointer_list.pointers, kSmallMonster);
  memory.store(
    kList + profile.pointer_list.pointers + sizeof(std::uint32_t), kMonster
  );
  memory.store(kList + profile.pointer_list.count, two);
  assert(reader.validate_pointer_list(kList));
  assert(reader.find_pointer_list() == kList);

  core::GameSnapshot small_first{};
  assert(reader.read_snapshot(kList, core::Locale::English, small_first));
  assert(small_first.monster_count == 1);
  assert(small_first.monsters[0].handle == kMonster);

  for (std::size_t index = 0; index < core::kMaxMonsters; ++index) {
    const auto pointer =
      index + 1 == core::kMaxMonsters ? kMonster : kSmallMonster;
    memory.store(
      kList + profile.pointer_list.pointers + index * sizeof(std::uint32_t),
      pointer
    );
  }
  const auto full_list = static_cast<std::uint8_t>(core::kMaxMonsters);
  memory.store(kList + profile.pointer_list.count, full_list);
  core::GameSnapshot crowded_with_small_monsters{};
  assert(reader.read_snapshot(
    kList, core::Locale::English, crowded_with_small_monsters
  ));
  assert(crowded_with_small_monsters.monster_count == 1);
  assert(crowded_with_small_monsters.monsters[0].handle == kMonster);

  const std::uint32_t no_pointer = 0;
  memory.store(kList + profile.pointer_list.pointers, no_pointer);
  assert(!reader.validate_pointer_list(kList));

  core::GameSnapshot missing_small_slot{};
  assert(reader.read_snapshot(
    kList, core::Locale::English, missing_small_slot
  ));
  assert(missing_small_slot.monster_count == 1);
  assert(missing_small_slot.monsters[0].handle == kMonster);

  const std::uint8_t one_monster = 1;
  memory.store(kList + profile.pointer_list.pointers, kSmallMonster);
  memory.store(kList + profile.pointer_list.count, one_monster);
  assert(!reader.validate_pointer_list(kList));

  core::GameSnapshot count_changed_before_slots{};
  assert(reader.read_snapshot(
    kList, core::Locale::English, count_changed_before_slots
  ));
  assert(count_changed_before_slots.monster_count == 1);
  assert(count_changed_before_slots.monsters[0].handle == kMonster);

  request.handle = kMonster;
  request.target_percent = rathian->mini_percent;
  assert(reader.apply_size(kList, request, verified));
  assert(verified == rathian->mini_percent);

  memory.store(kList + profile.pointer_list.pointers, kMonster);
  memory.store(
    kList + profile.pointer_list.pointers + sizeof(std::uint32_t), kMonster
  );
  memory.store(kList + profile.pointer_list.count, two);
  core::GameSnapshot duplicate_pointer{};
  assert(reader.read_snapshot(
    kList, core::Locale::English, duplicate_pointer
  ));
  assert(duplicate_pointer.monster_count == 1);
  assert(duplicate_pointer.monsters[0].handle == kMonster);

  const std::uint8_t no_monsters = 0;
  for (std::size_t index = 0; index < core::kMaxMonsters; ++index) {
    memory.store(
      kList + profile.pointer_list.pointers + index * sizeof(std::uint32_t),
      no_pointer
    );
  }
  memory.store(kList + profile.pointer_list.count, no_monsters);
  assert(!reader.validate_pointer_list(kList));

  core::GameSnapshot temporarily_empty{};
  assert(reader.read_snapshot(
    kList, core::Locale::English, temporarily_empty
  ));
  assert(temporarily_empty.monster_count == 0);

  std::cout << "switch adapter tests passed\n";
}
