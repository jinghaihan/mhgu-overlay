#pragma once

#include <cstddef>
#include <cstdint>

#include "mhgu/core/types.hpp"
#include "mhgu/platform/switch/game_profile.hpp"
#include "mhgu/platform/switch/memory.hpp"

namespace mhgu::platform::switch_adapter {

struct ResolvedMonster {
  core::MonsterId monster_id{};
  std::uint32_t raw_id{};
  bool hyper{};
};

std::uint32_t normalized_raw_id(
  std::uint16_t primary_identifier, std::uint8_t secondary_identifier
);

ResolvedMonster resolve_monster(
  std::uint16_t primary_identifier, std::uint8_t secondary_identifier
);

class MonsterReader {
public:
  MonsterReader(
    MemoryAccess& memory,
    const GameProfile& profile,
    std::uint64_t heap_base,
    std::uint64_t heap_size
  );

  std::uint64_t find_pointer_list();
  bool validate_pointer_list(std::uint64_t address);
  bool read_snapshot(
    std::uint64_t pointer_list_address,
    core::Locale detected_locale,
    core::GameSnapshot& snapshot
  );
  bool read_health(
    core::MonsterHandle handle,
    std::uint32_t expected_max_hp,
    std::uint32_t& health
  );
  bool apply_size(
    std::uint64_t pointer_list_address,
    const core::SizeWriteRequest& request,
    std::uint16_t& verified_percent
  );

private:
  bool read_monster(std::uint64_t address, core::MonsterSnapshot& snapshot);
  bool monster_identity(std::uint64_t address, ResolvedMonster& resolved);
  bool pointer_list_contains(
    std::uint64_t pointer_list_address, core::MonsterHandle handle
  );
  bool contains_heap_range(std::uint64_t address, std::size_t size) const;
  bool contains_monster(std::uint64_t address) const;

  MemoryAccess& memory_;
  const GameProfile& profile_;
  std::uint64_t heap_base_;
  std::uint64_t heap_size_;
};

}  // namespace mhgu::platform::switch_adapter
