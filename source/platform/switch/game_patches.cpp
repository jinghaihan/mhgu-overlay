#include "mhgu/platform/switch/game_patches.hpp"

#include <array>
#include <cstring>
#include <limits>

namespace mhgu::platform::switch_adapter {
namespace {

bool checked_add(
  const std::uint64_t base,
  const std::uint64_t offset,
  std::uint64_t& result
) {
  if (offset > std::numeric_limits<std::uint64_t>::max() - base) {
    return false;
  }
  result = base + offset;
  return true;
}

bool encode_numeric_word_patch(
  const NumericWordPatch& patch,
  const std::uint32_t input,
  MainWordPatch& encoded
) {
  encoded.offset = patch.offset;
  if (patch.encoding == NumericWordEncoding::Fixed) {
    encoded.value = patch.base_value;
    return true;
  }
  if (patch.encoding == NumericWordEncoding::FloatTenths) {
    static_assert(sizeof(float) == sizeof(std::uint32_t));
    static_assert(std::numeric_limits<float>::is_iec559);
    const auto multiplier = static_cast<float>(input) / 10.0F;
    std::memcpy(&encoded.value, &multiplier, sizeof(encoded.value));
    return true;
  }
  if (patch.encoding == NumericWordEncoding::ArmMovwImmediate ||
      patch.encoding == NumericWordEncoding::ArmMovtImmediate) {
    constexpr std::uint32_t kArmImmediateMask = 0x000F0FFF;
    if ((patch.base_value & kArmImmediateMask) != 0) {
      return false;
    }
    std::uint32_t immediate{};
    if (patch.encoding == NumericWordEncoding::ArmMovwImmediate) {
      immediate = input & 0xFFFFU;
    } else {
      immediate = input >> 16;
    }
    encoded.value = patch.base_value |
                    ((immediate & 0xF000U) << 4) |
                    (immediate & 0x0FFFU);
    return true;
  }
  if (patch.encoding != NumericWordEncoding::LinearImmediate &&
      patch.encoding != NumericWordEncoding::LinearWord) {
    return false;
  }

  const auto computed =
    static_cast<std::int64_t>(input) * patch.multiplier + patch.addend;
  if (computed < 0 ||
      computed > std::numeric_limits<std::uint32_t>::max()) {
    return false;
  }
  if (patch.encoding == NumericWordEncoding::LinearWord) {
    encoded.value = static_cast<std::uint32_t>(computed);
    return true;
  }
  if ((patch.base_value & 0xFFU) != 0 || computed > 0xFF) {
    return false;
  }
  encoded.value = patch.base_value | static_cast<std::uint32_t>(computed);
  return true;
}

}  // namespace

GamePatches::GamePatches(
  MemoryAccess& memory,
  const GameProfile& profile,
  const std::uint64_t main_base,
  const std::uint64_t main_size,
  const std::uint64_t address_space_base,
  const std::uint64_t address_space_size,
  const std::uint64_t heap_base,
  const std::uint64_t heap_size
)
  : memory_(memory),
    profile_(profile),
    main_base_(main_base),
    main_size_(main_size),
    address_space_base_(address_space_base),
    address_space_size_(address_space_size),
    heap_base_(heap_base),
    heap_size_(heap_size) {}

bool GamePatches::contains(
  const std::uint64_t region_base,
  const std::uint64_t region_size,
  const std::uint64_t address,
  const std::size_t size
) const {
  if (address < region_base || size > region_size) {
    return false;
  }
  return address - region_base <= region_size - size;
}

bool GamePatches::main_word_patch_address(
  const MainWordPatch& patch, std::uint64_t& address
) const {
  return checked_add(main_base_, patch.offset, address) &&
         contains(main_base_, main_size_, address, sizeof(std::uint32_t));
}

bool GamePatches::apply_main_word_patch(
  const MainWordPatch& patch, const std::uint64_t address
) {
  std::uint32_t current{};
  if (!memory_.read(address, &current, sizeof(current))) {
    return false;
  }
  if (current == patch.value) {
    return true;
  }
  if (!memory_.write(address, &patch.value, sizeof(patch.value))) {
    return false;
  }

  std::uint32_t verified{};
  return memory_.read(address, &verified, sizeof(verified)) &&
         verified == patch.value;
}

QuestOperationResult GamePatches::quest_base(std::uint64_t& base) {
  std::uint64_t pointer_address{};
  if (!checked_add(
        main_base_, profile_.quest.pointer_from_main, pointer_address
      ) ||
      !contains(
        main_base_, main_size_, pointer_address, sizeof(std::uint32_t)
      )) {
    return QuestOperationResult::Failed;
  }

  std::uint32_t pointer{};
  if (!memory_.read(pointer_address, &pointer, sizeof(pointer))) {
    return QuestOperationResult::Failed;
  }
  if (pointer == 0) {
    return QuestOperationResult::NoActiveQuest;
  }
  base = pointer;
  return contains(address_space_base_, address_space_size_, base, 1)
           ? QuestOperationResult::Success
           : QuestOperationResult::Failed;
}

bool GamePatches::quest_address(
  const std::uint64_t base,
  const std::uint64_t offset,
  const std::size_t size,
  std::uint64_t& address
) const {
  return checked_add(base, offset, address) &&
         contains(address_space_base_, address_space_size_, address, size);
}

bool GamePatches::apply_value(
  const std::uint64_t address,
  const void* value,
  const std::size_t size
) {
  if (size == 0 || size > sizeof(std::uint32_t) ||
      !contains(address_space_base_, address_space_size_, address, size)) {
    return false;
  }

  std::array<std::uint8_t, sizeof(std::uint32_t)> current{};
  if (!memory_.read(address, current.data(), size)) {
    return false;
  }
  if (std::memcmp(current.data(), value, size) == 0) {
    return true;
  }
  if (!memory_.write(address, value, size)) {
    return false;
  }

  std::array<std::uint8_t, sizeof(std::uint32_t)> verified{};
  return memory_.read(address, verified.data(), size) &&
         std::memcmp(verified.data(), value, size) == 0;
}

bool GamePatches::set_frame_rate(const core::FrameRate frame_rate) {
  const auto& patch = profile_.frame_rate;
  std::uint64_t pointer_address{};
  if (!checked_add(main_base_, patch.pointer_from_main, pointer_address) ||
      !contains(
        main_base_, main_size_, pointer_address, sizeof(std::uint64_t)
      )) {
    return false;
  }

  // This mirrors Atmosphere's `58000000` code type: load an eight-byte value
  // from Main + pointer_from_main before applying the `78000000` offset.
  std::uint64_t target_base{};
  if (!memory_.read(pointer_address, &target_base, sizeof(target_base))) {
    return false;
  }

  std::uint64_t target_address{};
  if (!checked_add(target_base, patch.target_from_pointer, target_address) ||
      !contains(
        address_space_base_,
        address_space_size_,
        target_address,
        sizeof(std::uint32_t)
      )) {
    return false;
  }

  std::uint32_t current{};
  if (!memory_.read(target_address, &current, sizeof(current))) {
    return false;
  }
  const auto desired = frame_rate == core::FrameRate::Fps60
                         ? patch.fps60_value
                         : patch.fps30_value;
  if (current == desired) {
    return true;
  }
  if (current != patch.fps30_value && current != patch.fps60_value) {
    return false;
  }
  if (!memory_.write(target_address, &desired, sizeof(desired))) {
    return false;
  }

  std::uint32_t verified{};
  return memory_.read(target_address, &verified, sizeof(verified)) &&
         verified == desired;
}

bool GamePatches::set_monster_damage_mode(
  const core::MonsterDamageMode mode
) {
  std::uint32_t value{};
  switch (mode) {
    case core::MonsterDamageMode::InstantKill:
      value = profile_.monster_damage.instant_kill_value;
      break;
    case core::MonsterDamageMode::LeaveOneHp:
      value = profile_.monster_damage.leave_one_hp_value;
      break;
    default:
      return false;
  }

  const MainWordPatch patch{profile_.monster_damage.offset, value};
  std::uint64_t address{};
  return main_word_patch_address(patch, address) &&
         apply_main_word_patch(patch, address);
}

bool GamePatches::set_item_pouch_quantity(
  const std::uint8_t slot, const std::uint8_t quantity
) {
  const auto& layout = profile_.item_pouch;
  if (slot == 0 || slot > layout.slot_count ||
      quantity < layout.minimum_quantity ||
      quantity > layout.maximum_quantity) {
    return false;
  }

  const auto slot_offset =
    static_cast<std::uint64_t>(slot - 1) * layout.slot_stride;
  std::uint64_t offset{};
  std::uint64_t address{};
  if (!checked_add(layout.first_quantity_from_heap, slot_offset, offset) ||
      !checked_add(heap_base_, offset, address) ||
      !contains(heap_base_, heap_size_, address, sizeof(quantity))) {
    return false;
  }

  std::uint8_t current{};
  if (!memory_.read(address, &current, sizeof(current))) {
    return false;
  }
  if (current == quantity) {
    return true;
  }
  if (!memory_.write(address, &quantity, sizeof(quantity))) {
    return false;
  }
  std::uint8_t verified{};
  return memory_.read(address, &verified, sizeof(verified)) &&
         verified == quantity;
}

QuestOperationResult GamePatches::maintain_quest(
  const bool infinite_time, const bool unlimited_faints
) {
  if (!infinite_time && !unlimited_faints) {
    return QuestOperationResult::Success;
  }

  std::uint64_t base{};
  const auto base_result = quest_base(base);
  if (base_result != QuestOperationResult::Success) {
    return base_result;
  }

  std::uint64_t time_address{};
  std::uint64_t faint_count_address{};
  std::uint64_t secondary_faint_count_address{};
  if ((infinite_time &&
       !quest_address(
         base,
         profile_.quest.time_from_quest,
         sizeof(profile_.quest.time_value),
         time_address
       )) ||
      (unlimited_faints &&
       (!quest_address(
          base,
          profile_.quest.faint_count_from_quest,
          sizeof(std::uint32_t),
          faint_count_address
        ) ||
        !quest_address(
          base,
          profile_.quest.secondary_faint_count_from_quest,
          sizeof(std::uint16_t),
          secondary_faint_count_address
        )))) {
    return QuestOperationResult::Failed;
  }

  if (infinite_time &&
      !apply_value(
        time_address,
        &profile_.quest.time_value,
        sizeof(profile_.quest.time_value)
      )) {
    return QuestOperationResult::Failed;
  }
  constexpr std::uint32_t kZero32{};
  constexpr std::uint16_t kZero16{};
  if (unlimited_faints &&
      (!apply_value(
         faint_count_address, &kZero32, sizeof(kZero32)
       ) ||
       !apply_value(
         secondary_faint_count_address, &kZero16, sizeof(kZero16)
       ))) {
    return QuestOperationResult::Failed;
  }
  return QuestOperationResult::Success;
}

QuestOperationResult GamePatches::complete_quest() {
  std::uint64_t base{};
  const auto base_result = quest_base(base);
  if (base_result != QuestOperationResult::Success) {
    return base_result;
  }

  std::uint64_t address{};
  if (!quest_address(
        base,
        profile_.quest.completion_state_from_quest,
        sizeof(profile_.quest.completion_value),
        address
      ) ||
      !apply_value(
        address,
        &profile_.quest.completion_value,
        sizeof(profile_.quest.completion_value)
      )) {
    return QuestOperationResult::Failed;
  }
  return QuestOperationResult::Success;
}

bool GamePatches::enable_runtime_feature(
  const core::RuntimeFeature feature
) {
  const auto feature_index = core::runtime_feature_index(feature);
  if (feature_index >= profile_.runtime_patches.size()) {
    return false;
  }
  const auto& patch_set = profile_.runtime_patches[feature_index];
  if (patch_set.count == 0 || patch_set.count > patch_set.patches.size()) {
    return false;
  }

  std::array<std::uint64_t, kMaxMainWordPatchesPerFeature> addresses{};
  for (std::size_t index = 0; index < patch_set.count; ++index) {
    if (!main_word_patch_address(patch_set.patches[index], addresses[index])) {
      return false;
    }
  }
  for (std::size_t index = 0; index < patch_set.count; ++index) {
    if (!apply_main_word_patch(
          patch_set.patches[index], addresses[index]
        )) {
      return false;
    }
  }
  return true;
}

bool GamePatches::set_numeric_feature(
  const core::NumericFeature feature, const std::uint32_t value
) {
  const auto feature_index = core::numeric_feature_index(feature);
  if (feature_index >= profile_.numeric_patches.size()) {
    return false;
  }
  const auto& patch_set = profile_.numeric_patches[feature_index];
  if (patch_set.count == 0 ||
      patch_set.count > patch_set.patches.size() ||
      value < patch_set.minimum || value > patch_set.maximum) {
    return false;
  }

  std::array<MainWordPatch, kMaxMainWordPatchesPerFeature> encoded{};
  std::array<std::uint64_t, kMaxMainWordPatchesPerFeature> addresses{};
  for (std::size_t index = 0; index < patch_set.count; ++index) {
    if (!encode_numeric_word_patch(
          patch_set.patches[index], value, encoded[index]
        ) ||
        !main_word_patch_address(encoded[index], addresses[index])) {
      return false;
    }
  }
  for (std::size_t index = 0; index < patch_set.count; ++index) {
    if (!apply_main_word_patch(encoded[index], addresses[index])) {
      return false;
    }
  }
  return true;
}

}  // namespace mhgu::platform::switch_adapter
