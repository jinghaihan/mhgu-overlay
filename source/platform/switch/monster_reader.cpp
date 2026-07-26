#include "mhgu/platform/switch/monster_reader.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <vector>

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

}  // namespace

std::uint32_t normalized_raw_id(
    std::uint16_t primary_identifier,
    const std::uint8_t secondary_identifier
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

    const auto raw_id = normalized_raw_id(
        primary_identifier,
        secondary_identifier
    );
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
    const std::uint64_t heap_base
) : memory_(memory), profile_(profile), heap_base_(heap_base) {}

std::uint64_t MonsterReader::find_pointer_list() {
    const auto begin = heap_base_ + profile_.scan_start_from_heap;
    const auto end = heap_base_ + profile_.scan_end_from_heap;
    const auto structure_size = profile_.pointer_list.byte_size;
    if (end <= begin || structure_size == 0) {
        return 0;
    }

    std::vector<std::uint8_t> buffer(
        kScanChunkSize + structure_size - 1
    );
    for (auto address = begin; address < end;) {
        const auto remaining = end - address;
        const auto body_size = static_cast<std::size_t>(
            std::min<std::uint64_t>(remaining, kScanChunkSize)
        );
        const auto read_size = std::min<std::size_t>(
            body_size + structure_size - 1,
            static_cast<std::size_t>(remaining)
        );
        if (!memory_.read(address, buffer.data(), read_size)) {
            address += body_size;
            continue;
        }

        for (std::size_t offset = 0;
             offset + structure_size <= read_size;
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

bool MonsterReader::validate_pointer_list(const std::uint64_t address) {
    std::array<std::uint8_t, 0x41> bytes{};
    if (profile_.pointer_list.byte_size > bytes.size() ||
        !memory_.read(
            address,
            bytes.data(),
            profile_.pointer_list.byte_size
        )) {
        return false;
    }

    const auto& layout = profile_.pointer_list;
    if (bytes[layout.marker] != 1 || bytes[layout.marker + 1] != 1) {
        return false;
    }
    for (std::uint32_t offset = layout.padding;
         offset < layout.pointers;
         ++offset) {
        if (bytes[offset] > 1) {
            return false;
        }
    }

    const auto count = bytes[layout.count];
    if (count == 0 || count > kPointerCapacity) {
        return false;
    }
    for (std::size_t index = 0; index < kPointerCapacity; ++index) {
        const auto pointer = read_u32(
            bytes.data() + layout.pointers + index * sizeof(std::uint32_t)
        );
        if ((index < count && pointer == 0) ||
            (index >= count && pointer != 0)) {
            return false;
        }
    }

    ResolvedMonster first{};
    const auto first_pointer = read_u32(bytes.data() + layout.pointers);
    return monster_identity(first_pointer, first) && first.monster_id != 0;
}

bool MonsterReader::monster_identity(
    const std::uint64_t address,
    ResolvedMonster& resolved
) {
    std::uint8_t secondary{};
    std::uint16_t primary{};
    if (!read_value(
            memory_,
            address + profile_.monster.secondary_identifier,
            secondary
        ) ||
        !read_value(
            memory_,
            address + profile_.monster.primary_identifier,
            primary
        )) {
        return false;
    }
    resolved = resolve_monster(primary, secondary);
    return resolved.monster_id != 0;
}

bool MonsterReader::read_monster(
    const std::uint64_t address,
    core::MonsterSnapshot& snapshot
) {
    ResolvedMonster resolved{};
    std::uint8_t location{};
    std::uint32_t health{};
    std::uint32_t maximum_health{};
    float size_multiplier{};
    if (!read_value(
            memory_,
            address + profile_.monster.location_flag,
            location
        ) ||
        location != profile_.monster.current_location_value ||
        !monster_identity(address, resolved) ||
        !read_value(
            memory_,
            address + profile_.monster.health,
            health
        ) ||
        !read_value(
            memory_,
            address + profile_.monster.maximum_health,
            maximum_health
        ) ||
        !read_value(
            memory_,
            address + profile_.monster.size_multiplier,
            size_multiplier
        )) {
        return false;
    }

    const auto size_percent = multiplier_percent(size_multiplier);
    if (maximum_health == 0 || maximum_health > 20'000'000U ||
        health > maximum_health || size_percent == 0) {
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
    if (layout.byte_size > bytes.size() ||
        !memory_.read(pointer_list_address, bytes.data(), layout.byte_size)) {
        return false;
    }

    const auto count = std::min<std::size_t>(
        bytes[layout.count],
        kPointerCapacity
    );
    for (std::size_t index = 0; index < count; ++index) {
        const auto pointer = read_u32(
            bytes.data() + layout.pointers + index * sizeof(std::uint32_t)
        );
        core::MonsterSnapshot monster{};
        if (pointer != 0 && read_monster(pointer, monster)) {
            snapshot.monsters[snapshot.monster_count++] = monster;
        }
    }
    return true;
}

bool MonsterReader::apply_size(
    const core::SizeWriteRequest& request,
    std::uint16_t& verified_percent
) {
    verified_percent = 0;
    ResolvedMonster resolved{};
    std::uint8_t location{};
    if (request.handle == 0 ||
        request.target_percent < 50 ||
        request.target_percent > 200 ||
        !read_value(
            memory_,
            request.handle + profile_.monster.location_flag,
            location
        ) ||
        location != profile_.monster.current_location_value ||
        !monster_identity(request.handle, resolved) ||
        resolved.monster_id != request.monster_id) {
        return false;
    }

    const auto address = request.handle + profile_.monster.size_multiplier;
    const auto multiplier =
        static_cast<float>(request.target_percent) / 100.0F;
    if (!memory_.write(address, &multiplier, sizeof(multiplier))) {
        return false;
    }

    float read_back{};
    if (!read_value(memory_, address, read_back)) {
        return false;
    }
    verified_percent = multiplier_percent(read_back);
    return verified_percent == request.target_percent;
}

}  // namespace mhgu::platform::switch_adapter
