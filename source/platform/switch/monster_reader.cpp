#include "mhgu/platform/switch/monster_reader.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <vector>

#include "mhgu/core/catalog.hpp"
#include "mhgu/core/size.hpp"
#include "mhgu/platform/switch/monster_aliases.hpp"

namespace mhgu::platform::switch_adapter {
namespace {

constexpr std::size_t kPointerCapacity = core::kMaxMonsters;
constexpr std::size_t kScanChunkSize = 64 * 1024;

template <typename T>
bool read_value(MemoryAccess& memory, const std::uint64_t address, T& value) {
  return memory.read(address, &value, sizeof(value));
}

std::uint32_t read_u32(const std::uint8_t* bytes) {
  std::uint32_t value{};
  std::memcpy(&value, bytes, sizeof(value));
  return value;
}

std::uint16_t multiplier_percent(const float multiplier) {
  if (!std::isfinite(multiplier) || multiplier < 0.50F || multiplier > 2.00F) {
    return 0;
  }
  return static_cast<std::uint16_t>(std::lround(multiplier * 100.0F));
}

bool is_known_location(
  const GameProfile& profile, const std::uint8_t location
) {
  return location == profile.monster.current_location_value ||
         location == profile.monster.remote_location_value;
}

bool is_live_health(
  const std::uint32_t health, const std::uint32_t maximum_health
) {
  return health != 0 && maximum_health != 0 && maximum_health <= 20'000'000U &&
         health <= maximum_health;
}

}  // namespace

std::uint32_t normalized_raw_id(
  std::uint16_t primary_identifier, const std::uint8_t secondary_identifier
) {
  switch (primary_identifier & 0x00FFU) {
    case 0x40:
    case 0xC0:
      primary_identifier =
        static_cast<std::uint16_t>(primary_identifier + 0x60);
      break;
    case 0x00:
    case 0x80:
      primary_identifier =
        static_cast<std::uint16_t>(primary_identifier + 0x05A0);
      break;
    default:
      break;
  }
  return (static_cast<std::uint32_t>(primary_identifier) << 8U) |
         secondary_identifier;
}

ResolvedMonster resolve_monster(
  const std::uint16_t primary_identifier,
  const std::uint8_t secondary_identifier
) {
  if (secondary_identifier == 0 || secondary_identifier == 0x80) {
    return {};
  }

  const auto raw_id =
    normalized_raw_id(primary_identifier, secondary_identifier);
  if (const auto* exact = find_monster_alias(raw_id); exact != nullptr) {
    return {exact->monster_id, raw_id, false};
  }

  if ((secondary_identifier & 0x08U) != 0) {
    if (const auto* base = find_monster_alias(raw_id - 0x08U);
        base != nullptr) {
      return {base->monster_id, raw_id, true};
    }
  }
  return {};
}

MonsterReader::MonsterReader(
  MemoryAccess& memory,
  const GameProfile& profile,
  const std::uint64_t heap_base,
  const std::uint64_t heap_size
)
  : memory_(memory),
    profile_(profile),
    heap_base_(heap_base),
    heap_size_(heap_size) {}

std::uint64_t MonsterReader::find_pointer_list() {
  const auto begin_offset = profile_.scan_start_from_heap;
  const auto end_offset = std::min(profile_.scan_end_from_heap, heap_size_);
  const auto structure_size = profile_.pointer_list.byte_size;
  if (end_offset <= begin_offset || structure_size == 0 ||
      heap_base_ > std::numeric_limits<std::uint64_t>::max() - end_offset ||
      !contains_heap_range(
        heap_base_ + begin_offset,
        static_cast<std::size_t>(end_offset - begin_offset)
      )) {
    return 0;
  }
  const auto begin = heap_base_ + begin_offset;
  const auto end = heap_base_ + end_offset;

  std::vector<std::uint8_t> buffer(kScanChunkSize + structure_size - 1);
  for (auto address = begin; address < end;) {
    const auto remaining = end - address;
    const auto body_size = static_cast<std::size_t>(
      std::min<std::uint64_t>(remaining, kScanChunkSize)
    );
    const auto read_size = std::min<std::size_t>(
      body_size + structure_size - 1, static_cast<std::size_t>(remaining)
    );
    if (!memory_.read(address, buffer.data(), read_size)) {
      address += body_size;
      continue;
    }

    for (std::size_t offset = 0; offset + structure_size <= read_size;
         offset += 2) {
      const auto candidate = address + offset;
      if (buffer[offset] != 1 || buffer[offset + 1] != 1) {
        continue;
      }
      if (validate_pointer_list(candidate)) {
        return candidate;
      }
    }
    address += body_size;
  }
  return 0;
}

bool MonsterReader::read_pointer_list(
  const std::uint64_t address,
  const bool allow_empty,
  std::array<std::uint8_t, 0x41>& bytes
) {
  bytes = {};
  if (profile_.pointer_list.byte_size > bytes.size() ||
      !contains_heap_range(address, profile_.pointer_list.byte_size) ||
      !memory_.read(address, bytes.data(), profile_.pointer_list.byte_size)) {
    return false;
  }

  const auto& layout = profile_.pointer_list;
  if (bytes[layout.marker] != 1 || bytes[layout.marker + 1] != 1) {
    return false;
  }
  for (std::uint32_t offset = layout.padding; offset < layout.pointers;
       ++offset) {
    if (bytes[offset] > 1) {
      return false;
    }
  }

  const auto count = bytes[layout.count];
  if ((!allow_empty && count == 0) || count > kPointerCapacity) {
    return false;
  }
  for (std::size_t index = 0; index < kPointerCapacity; ++index) {
    const auto pointer =
      read_u32(bytes.data() + layout.pointers + index * sizeof(std::uint32_t));
    if ((index < count && (pointer == 0 || !contains_monster(pointer))) ||
        (index >= count && pointer != 0)) {
      return false;
    }
  }
  return true;
}

bool MonsterReader::plausible_monster(const std::uint64_t address) {
  std::uint8_t location{};
  std::uint8_t secondary{};
  std::uint32_t health{};
  std::uint32_t maximum_health{};
  if (!contains_monster(address) ||
      !read_value(memory_, address + profile_.monster.location_flag, location) ||
      !is_known_location(profile_, location) ||
      !read_value(
        memory_, address + profile_.monster.secondary_identifier, secondary
      ) ||
      !read_value(memory_, address + profile_.monster.health, health) ||
      !read_value(
        memory_, address + profile_.monster.maximum_health, maximum_health
      ) ||
      maximum_health == 0 || maximum_health > 20'000'000U ||
      health > maximum_health) {
    return false;
  }

  if (secondary == 0 || secondary == 0x80) {
    return true;
  }

  ResolvedMonster resolved{};
  float size_multiplier{};
  return monster_identity(address, resolved) && resolved.monster_id != 0 &&
         read_value(
           memory_, address + profile_.monster.size_multiplier, size_multiplier
         ) &&
         multiplier_percent(size_multiplier) != 0;
}

bool MonsterReader::validate_pointer_list(const std::uint64_t address) {
  std::array<std::uint8_t, 0x41> bytes{};
  if (!read_pointer_list(address, false, bytes)) {
    return false;
  }

  const auto& layout = profile_.pointer_list;
  const auto count = bytes[layout.count];
  for (std::size_t index = 0; index < count; ++index) {
    const auto pointer =
      read_u32(bytes.data() + layout.pointers + index * sizeof(std::uint32_t));
    if (plausible_monster(pointer)) {
      return true;
    }
  }
  return false;
}

bool MonsterReader::monster_identity(
  const std::uint64_t address, ResolvedMonster& resolved
) {
  if (!contains_monster(address)) {
    return false;
  }
  std::uint8_t secondary{};
  std::uint16_t primary{};
  if (!read_value(
        memory_, address + profile_.monster.secondary_identifier, secondary
      ) ||
      !read_value(
        memory_, address + profile_.monster.primary_identifier, primary
      )) {
    return false;
  }
  resolved = resolve_monster(primary, secondary);
  return resolved.monster_id != 0;
}

bool MonsterReader::pointer_list_contains(
  const std::uint64_t pointer_list_address, const core::MonsterHandle handle
) {
  if (handle == 0) {
    return false;
  }

  std::array<std::uint8_t, 0x41> bytes{};
  const auto& layout = profile_.pointer_list;
  if (!read_pointer_list(pointer_list_address, false, bytes)) {
    return false;
  }

  const auto count =
    std::min<std::size_t>(bytes[layout.count], kPointerCapacity);
  for (std::size_t index = 0; index < count; ++index) {
    const auto pointer =
      read_u32(bytes.data() + layout.pointers + index * sizeof(std::uint32_t));
    if (pointer == handle) {
      return true;
    }
  }
  return false;
}

bool MonsterReader::read_monster(
  const std::uint64_t address, core::MonsterSnapshot& snapshot
) {
  if (!contains_monster(address)) {
    return false;
  }
  ResolvedMonster resolved{};
  std::uint8_t location{};
  std::uint32_t health{};
  std::uint32_t maximum_health{};
  float size_multiplier{};
  if (!read_value(
        memory_, address + profile_.monster.location_flag, location
      ) ||
      !is_known_location(profile_, location) ||
      !monster_identity(address, resolved) ||
      !read_value(memory_, address + profile_.monster.health, health) ||
      !read_value(
        memory_, address + profile_.monster.maximum_health, maximum_health
      ) ||
      !read_value(
        memory_, address + profile_.monster.size_multiplier, size_multiplier
      )) {
    return false;
  }

  const auto size_percent = multiplier_percent(size_multiplier);
  if (!is_live_health(health, maximum_health) || size_percent == 0) {
    return false;
  }
  snapshot = {
    address,
    resolved.monster_id,
    health,
    maximum_health,
    size_percent,
    resolved.hyper,
  };
  return true;
}

bool MonsterReader::read_snapshot(
  const std::uint64_t pointer_list_address,
  const core::Locale detected_locale,
  core::GameSnapshot& snapshot
) {
  snapshot = {};
  snapshot.game = profile_.game;
  snapshot.detected_locale = detected_locale;

  std::array<std::uint8_t, 0x41> bytes{};
  const auto& layout = profile_.pointer_list;
  if (!read_pointer_list(pointer_list_address, true, bytes)) {
    return false;
  }

  const auto count =
    std::min<std::size_t>(bytes[layout.count], kPointerCapacity);
  for (std::size_t index = 0; index < count; ++index) {
    const auto pointer =
      read_u32(bytes.data() + layout.pointers + index * sizeof(std::uint32_t));
    core::MonsterSnapshot monster{};
    if (pointer != 0 && read_monster(pointer, monster)) {
      snapshot.monsters[snapshot.monster_count++] = monster;
    }
  }
  return true;
}

bool MonsterReader::read_health(
  const core::MonsterHandle handle,
  const std::uint32_t expected_max_hp,
  std::uint32_t& health
) {
  health = 0;
  if (handle == 0 || expected_max_hp == 0 ||
      expected_max_hp > 20'000'000U || !contains_monster(handle)) {
    return false;
  }

  std::uint32_t current{};
  std::uint32_t maximum{};
  const auto health_address = handle + profile_.monster.health;
  if (profile_.monster.maximum_health ==
      profile_.monster.health + sizeof(std::uint32_t)) {
    std::array<std::uint8_t, sizeof(std::uint32_t) * 2> values{};
    if (!memory_.read(health_address, values.data(), values.size())) {
      return false;
    }
    current = read_u32(values.data());
    maximum = read_u32(values.data() + sizeof(std::uint32_t));
  } else if (!read_value(memory_, health_address, current) ||
             !read_value(
               memory_,
               handle + profile_.monster.maximum_health,
               maximum
             )) {
    return false;
  }

  if (maximum != expected_max_hp || current > maximum) {
    return false;
  }
  health = current;
  return true;
}

bool MonsterReader::apply_size(
  const std::uint64_t pointer_list_address,
  const core::SizeWriteRequest& request,
  std::uint16_t& verified_percent
) {
  verified_percent = 0;
  ResolvedMonster resolved{};
  std::uint8_t location{};
  std::uint32_t health{};
  std::uint32_t maximum_health{};
  float current_multiplier{};
  const auto* definition = core::find_monster(request.monster_id);
  if (request.handle == 0 || !contains_monster(request.handle) ||
      !pointer_list_contains(pointer_list_address, request.handle) ||
      definition == nullptr ||
      !core::is_legal_size_percent(*definition, request.target_percent) ||
      !read_value(
        memory_, request.handle + profile_.monster.location_flag, location
      ) ||
      !is_known_location(profile_, location) ||
      !monster_identity(request.handle, resolved) ||
      resolved.monster_id != request.monster_id ||
      !read_value(memory_, request.handle + profile_.monster.health, health) ||
      !read_value(
        memory_,
        request.handle + profile_.monster.maximum_health,
        maximum_health
      ) ||
      !read_value(
        memory_,
        request.handle + profile_.monster.size_multiplier,
        current_multiplier
      )) {
    return false;
  }

  const auto current_percent = multiplier_percent(current_multiplier);
  if (!is_live_health(health, maximum_health) || current_percent == 0) {
    return false;
  }
  if (current_percent == request.target_percent) {
    verified_percent = request.target_percent;
    return true;
  }

  const auto address = request.handle + profile_.monster.size_multiplier;
  const auto multiplier = static_cast<float>(request.target_percent) / 100.0F;
  if (!memory_.write(address, &multiplier, sizeof(multiplier))) {
    return false;
  }

  float read_back{};
  if (!read_value(memory_, address, read_back)) {
    return false;
  }
  verified_percent = multiplier_percent(read_back);
  if (verified_percent != request.target_percent) {
    return false;
  }

  ResolvedMonster verified_identity{};
  std::uint8_t verified_location{};
  if (!read_value(
        memory_,
        request.handle + profile_.monster.location_flag,
        verified_location
      ) ||
      !is_known_location(profile_, verified_location) ||
      !monster_identity(request.handle, verified_identity)) {
    verified_percent = 0;
    return false;
  }
  if (verified_identity.monster_id != request.monster_id) {
    verified_percent = 0;
    return false;
  }
  return true;
}

bool MonsterReader::contains_heap_range(
  const std::uint64_t address, const std::size_t size
) const {
  if (address < heap_base_) {
    return false;
  }
  const auto offset = address - heap_base_;
  return offset <= heap_size_ && size <= heap_size_ - offset;
}

bool MonsterReader::contains_monster(const std::uint64_t address) const {
  const auto last_field = std::max({
    profile_.monster.location_flag + sizeof(std::uint8_t),
    profile_.monster.secondary_identifier + sizeof(std::uint8_t),
    profile_.monster.size_multiplier + sizeof(float),
    profile_.monster.health + sizeof(std::uint32_t),
    profile_.monster.maximum_health + sizeof(std::uint32_t),
    profile_.monster.primary_identifier + sizeof(std::uint16_t),
  });
  return contains_heap_range(address, last_field);
}

}  // namespace mhgu::platform::switch_adapter
