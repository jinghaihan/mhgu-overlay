#pragma once

#include <cstdint>

#include "mhgu/core/types.hpp"

namespace mhgu::platform::switch_adapter {

struct MonsterLayout {
    std::uint32_t location_flag;
    std::uint32_t secondary_identifier;
    std::uint32_t size_multiplier;
    std::uint32_t health;
    std::uint32_t maximum_health;
    std::uint32_t primary_identifier;
};

struct PointerListLayout {
    std::uint32_t marker;
    std::uint32_t padding;
    std::uint32_t pointers;
    std::uint32_t count;
    std::uint32_t byte_size;
};

struct GameProfile {
    const char* name;
    core::GameId game;
    std::uint64_t title_id;
    std::uint64_t scan_start_from_heap;
    std::uint64_t scan_end_from_heap;
    MonsterLayout monster;
    PointerListLayout pointer_list;
};

constexpr std::uint64_t kMhguTitleId = 0x0100770008DD8000ULL;
constexpr std::uint64_t kMhxxTitleId = 0x0100C3800049C000ULL;

const GameProfile* profile_for_title(std::uint64_t title_id);

}  // namespace mhgu::platform::switch_adapter
