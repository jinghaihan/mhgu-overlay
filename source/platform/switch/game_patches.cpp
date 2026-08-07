#include "mhgu/platform/switch/game_patches.hpp"

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

}  // namespace

GamePatches::GamePatches(
  MemoryAccess& memory,
  const GameProfile& profile,
  const std::uint64_t main_base,
  const std::uint64_t main_size,
  const std::uint64_t address_space_base,
  const std::uint64_t address_space_size
)
  : memory_(memory),
    profile_(profile),
    main_base_(main_base),
    main_size_(main_size),
    address_space_base_(address_space_base),
    address_space_size_(address_space_size) {}

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

bool GamePatches::set_frame_rate(const core::FrameRate frame_rate) {
  const auto& patch = profile_.frame_rate;
  std::uint64_t pointer_address{};
  if (!checked_add(main_base_, patch.pointer_from_main, pointer_address) ||
      !contains(
        main_base_, main_size_, pointer_address, sizeof(std::uint64_t)
      )) {
    return false;
  }

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

bool GamePatches::enable_map_and_large_monsters() {
  std::uint64_t show_map_address{};
  std::uint64_t mark_monsters_address{};
  if (!main_word_patch_address(profile_.show_map, show_map_address) ||
      !main_word_patch_address(
        profile_.mark_large_monsters, mark_monsters_address
      )) {
    return false;
  }
  return apply_main_word_patch(profile_.show_map, show_map_address) &&
         apply_main_word_patch(
           profile_.mark_large_monsters, mark_monsters_address
         );
}

}  // namespace mhgu::platform::switch_adapter
