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

bool GamePatches::read_patch_words(
  const MainWordPatch* patches,
  const std::size_t count,
  std::array<std::uint64_t, kMaxMainWordPatchesPerFeature>& addresses,
  std::array<std::uint32_t, kMaxMainWordPatchesPerFeature>& values
) {
  if (patches == nullptr || count == 0 || count > addresses.size()) {
    return false;
  }
  for (std::size_t index = 0; index < count; ++index) {
    if (!main_word_patch_address(patches[index], addresses[index]) ||
        !memory_.read(
          addresses[index], &values[index], sizeof(values[index])
        )) {
      return false;
    }
  }
  return true;
}

bool GamePatches::write_patch_words(
  const MainWordPatch* patches,
  const std::array<std::uint64_t, kMaxMainWordPatchesPerFeature>& addresses,
  const std::array<std::uint32_t, kMaxMainWordPatchesPerFeature>& previous,
  const std::size_t count
) {
  std::size_t attempted{};
  bool failed{};
  for (std::size_t index = 0; index < count; ++index) {
    if (previous[index] == patches[index].value) {
      continue;
    }
    attempted = index + 1;
    if (!memory_.write(
          addresses[index], &patches[index].value, sizeof(patches[index].value)
        )) {
      failed = true;
      break;
    }
    std::uint32_t verified{};
    if (!memory_.read(addresses[index], &verified, sizeof(verified)) ||
        verified != patches[index].value) {
      failed = true;
      break;
    }
  }
  if (!failed) {
    return true;
  }
  for (std::size_t index = 0; index < attempted; ++index) {
    if (previous[index] == patches[index].value) {
      continue;
    }
    memory_.write(addresses[index], &previous[index], sizeof(previous[index]));
  }
  return false;
}

bool GamePatches::baseline_patch_set(
  const MainWordPatch* patches,
  const std::size_t count,
  std::array<MainWordPatch, kMaxMainWordPatchesPerFeature>& baseline
) const {
  if (!baseline_ready_ || patches == nullptr || count == 0 ||
      count > baseline.size()) {
    return false;
  }
  for (std::size_t index = 0; index < count; ++index) {
    const auto* entry = baseline_.find(patches[index].offset);
    if (entry == nullptr) {
      return false;
    }
    baseline[index] = {entry->offset, entry->value};
  }
  return true;
}

bool GamePatches::set_baseline(const PatchBaseline& baseline) {
  if (!patch_baseline_matches_profile(baseline, profile_)) {
    return false;
  }
  baseline_ = baseline;
  baseline_ready_ = true;
  return true;
}

bool GamePatches::capture_baseline(
  const core::CoreSettings& settings, PatchBaseline& baseline
) {
  std::array<std::uint64_t, kMaxPatchBaselineEntries> offsets{};
  const auto count = collect_patch_offsets(profile_, offsets);
  if (count == 0 || !memory_.pause()) {
    return false;
  }

  auto patch_set_is_applied = [this](
                                const MainWordPatch* patches,
                                const std::size_t patch_count
                              ) {
    if (patch_count == 0 ||
        patch_count > kMaxMainWordPatchesPerFeature) {
      return false;
    }
    for (std::size_t index = 0; index < patch_count; ++index) {
      std::uint64_t address{};
      std::uint32_t current{};
      if (!main_word_patch_address(patches[index], address) ||
          !memory_.read(address, &current, sizeof(current)) ||
          current != patches[index].value) {
        return false;
      }
    }
    return true;
  };

  const MainWordPatch instant_kill{
    profile_.monster_damage.offset,
    profile_.monster_damage.instant_kill_value,
  };
  const MainWordPatch leave_one_hp{
    profile_.monster_damage.offset,
    profile_.monster_damage.leave_one_hp_value,
  };
  bool already_patched = patch_set_is_applied(&instant_kill, 1) ||
                         patch_set_is_applied(&leave_one_hp, 1);
  for (std::size_t index = 0;
       !already_patched && index < profile_.runtime_patches.size();
       ++index) {
    const auto& patch_set = profile_.runtime_patches[index];
    already_patched = patch_set_is_applied(
      patch_set.patches.data(), patch_set.count
    );
  }
  for (std::size_t index = 0;
       !already_patched && index < settings.numeric_features.size();
       ++index) {
    const auto& patch_set = profile_.numeric_patches[index];
    std::array<MainWordPatch, kMaxMainWordPatchesPerFeature> encoded{};
    bool valid = patch_set.count > 0 &&
                 patch_set.count <= patch_set.patches.size();
    for (std::size_t patch_index = 0;
         valid && patch_index < patch_set.count;
         ++patch_index) {
      valid = encode_numeric_word_patch(
        patch_set.patches[patch_index],
        settings.numeric_features[index].value,
        encoded[patch_index]
      );
    }
    already_patched = valid &&
                      patch_set_is_applied(encoded.data(), patch_set.count);
  }

  PatchBaseline captured{};
  captured.title_id = profile_.title_id;
  captured.build_id_prefix = profile_.build_id_prefix;
  captured.count = count;
  bool success = !already_patched;
  for (std::size_t index = 0; success && index < count; ++index) {
    const MainWordPatch target{offsets[index], 0};
    std::uint64_t address{};
    captured.entries[index].offset = offsets[index];
    success = main_word_patch_address(target, address) &&
              memory_.read(
                address,
                &captured.entries[index].value,
                sizeof(captured.entries[index].value)
              );
  }
  const auto resumed = memory_.resume();
  if (!success || !resumed ||
      !patch_baseline_matches_profile(captured, profile_)) {
    return false;
  }
  baseline = captured;
  return true;
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
  std::array<MainWordPatch, kMaxMainWordPatchesPerFeature> original{};
  const MainWordPatch enabled_patch{profile_.monster_damage.offset, 0};
  if (!baseline_patch_set(&enabled_patch, 1, original)) {
    return false;
  }
  std::uint32_t value = original[0].value;
  switch (mode) {
    case core::MonsterDamageMode::InstantKill:
      value = profile_.monster_damage.instant_kill_value;
      break;
    case core::MonsterDamageMode::LeaveOneHp:
      value = profile_.monster_damage.leave_one_hp_value;
      break;
    case core::MonsterDamageMode::Off:
      break;
    default:
      return false;
  }

  const MainWordPatch patch{profile_.monster_damage.offset, value};
  if (!memory_.pause()) {
    return false;
  }
  std::array<std::uint64_t, kMaxMainWordPatchesPerFeature> addresses{};
  std::array<std::uint32_t, kMaxMainWordPatchesPerFeature> current{};
  bool success = read_patch_words(&patch, 1, addresses, current) &&
                 (current[0] == original[0].value ||
                  current[0] == profile_.monster_damage.instant_kill_value ||
                  current[0] == profile_.monster_damage.leave_one_hp_value) &&
                 write_patch_words(&patch, addresses, current, 1);
  if (!memory_.resume()) {
    success = false;
  }
  return success;
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

bool GamePatches::set_runtime_feature(
  const core::RuntimeFeature feature, const bool enabled
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
  std::array<std::uint32_t, kMaxMainWordPatchesPerFeature> current{};
  std::array<MainWordPatch, kMaxMainWordPatchesPerFeature> original{};
  if (!baseline_patch_set(
        patch_set.patches.data(), patch_set.count, original
      ) ||
      !memory_.pause()) {
    return false;
  }
  bool success = read_patch_words(
    patch_set.patches.data(), patch_set.count, addresses, current
  );
  bool all_original = success;
  bool all_enabled = success;
  for (std::size_t index = 0; index < patch_set.count; ++index) {
    all_original = all_original && current[index] == original[index].value;
    all_enabled = all_enabled &&
                  current[index] == patch_set.patches[index].value;
  }
  const auto* desired = enabled ? patch_set.patches.data() : original.data();
  success = success && (all_original || all_enabled) &&
            write_patch_words(desired, addresses, current, patch_set.count);
  if (!memory_.resume()) {
    success = false;
  }
  return success;
}

bool GamePatches::enable_runtime_feature(
  const core::RuntimeFeature feature
) {
  return set_runtime_feature(feature, true);
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
  std::array<MainWordPatch, kMaxMainWordPatchesPerFeature> original{};
  std::array<std::uint64_t, kMaxMainWordPatchesPerFeature> addresses{};
  for (std::size_t index = 0; index < patch_set.count; ++index) {
    if (!encode_numeric_word_patch(
          patch_set.patches[index], value, encoded[index]
        ) ||
        !main_word_patch_address(encoded[index], addresses[index])) {
      return false;
    }
  }
  if (!baseline_patch_set(encoded.data(), patch_set.count, original) ||
      !memory_.pause()) {
    return false;
  }
  std::array<std::uint32_t, kMaxMainWordPatchesPerFeature> current{};
  bool success = read_patch_words(
    encoded.data(), patch_set.count, addresses, current
  );
  bool all_original = success;
  bool all_desired = success;
  bool all_previous =
    success && active_numeric_patch_counts_[feature_index] == patch_set.count;
  for (std::size_t index = 0; index < patch_set.count; ++index) {
    all_original = all_original && current[index] == original[index].value;
    all_desired = all_desired && current[index] == encoded[index].value;
    all_previous = all_previous &&
                   current[index] ==
                     active_numeric_patches_[feature_index][index].value;
  }
  success = success && (all_original || all_desired || all_previous) &&
            write_patch_words(
              encoded.data(), addresses, current, patch_set.count
            );
  if (!memory_.resume()) {
    success = false;
  }
  if (!success) {
    return false;
  }
  active_numeric_patches_[feature_index] = encoded;
  active_numeric_patch_counts_[feature_index] = patch_set.count;
  return true;
}

bool GamePatches::disable_numeric_feature(
  const core::NumericFeature feature, const std::uint32_t last_value
) {
  const auto feature_index = core::numeric_feature_index(feature);
  if (feature_index >= profile_.numeric_patches.size()) {
    return false;
  }
  const auto& patch_set = profile_.numeric_patches[feature_index];
  if (patch_set.count == 0 || patch_set.count > patch_set.patches.size()) {
    return false;
  }
  std::array<MainWordPatch, kMaxMainWordPatchesPerFeature> targets{};
  std::array<MainWordPatch, kMaxMainWordPatchesPerFeature> last_encoded{};
  for (std::size_t index = 0; index < patch_set.count; ++index) {
    targets[index].offset = patch_set.patches[index].offset;
    if (!encode_numeric_word_patch(
          patch_set.patches[index], last_value, last_encoded[index]
        )) {
      return false;
    }
  }
  std::array<MainWordPatch, kMaxMainWordPatchesPerFeature> original{};
  if (!baseline_patch_set(
        targets.data(), patch_set.count, original
      )) {
    return false;
  }
  if (!memory_.pause()) {
    return false;
  }
  std::array<std::uint64_t, kMaxMainWordPatchesPerFeature> addresses{};
  std::array<std::uint32_t, kMaxMainWordPatchesPerFeature> current{};
  bool success = read_patch_words(
    targets.data(), patch_set.count, addresses, current
  );
  bool all_original = success;
  bool all_previous =
    success && active_numeric_patch_counts_[feature_index] == patch_set.count;
  bool all_last_encoded = success;
  for (std::size_t index = 0; index < patch_set.count; ++index) {
    all_original = all_original && current[index] == original[index].value;
    all_previous = all_previous &&
                   current[index] ==
                     active_numeric_patches_[feature_index][index].value;
    all_last_encoded = all_last_encoded &&
                       current[index] == last_encoded[index].value;
  }
  success = success &&
            (all_original || all_previous || all_last_encoded) &&
            write_patch_words(
              original.data(), addresses, current, patch_set.count
            );
  if (!memory_.resume()) {
    success = false;
  }
  if (success) {
    active_numeric_patch_counts_[feature_index] = 0;
  }
  return success;
}

}  // namespace mhgu::platform::switch_adapter
