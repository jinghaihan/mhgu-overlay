#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

#include "mhgu/core/catalog.hpp"
#include "mhgu/platform/switch/game_profile.hpp"
#include "mhgu/platform/switch/language.hpp"
#include "mhgu/platform/switch/memory.hpp"
#include "mhgu/platform/switch/monster_reader.hpp"

namespace {

class FakeMemory final : public mhgu::platform::switch_adapter::MemoryAccess {
public:
    explicit FakeMemory(const std::size_t size) : bytes_(size) {}

    bool read(
        const std::uint64_t address,
        void* destination,
        const std::size_t size
    ) override {
        if (address > bytes_.size() || size > bytes_.size() - address) {
            return false;
        }
        std::memcpy(destination, bytes_.data() + address, size);
        return true;
    }

    bool write(
        const std::uint64_t address,
        const void* source,
        const std::size_t size
    ) override {
        if (address > bytes_.size() || size > bytes_.size() - address) {
            return false;
        }
        std::memcpy(bytes_.data() + address, source, size);
        return true;
    }

    template <typename T>
    void store(const std::size_t address, const T& value) {
        assert(write(address, &value, sizeof(value)));
    }

private:
    std::vector<std::uint8_t> bytes_;
};

}  // namespace

int main() {
    using namespace mhgu;
    using namespace platform::switch_adapter;

    std::array<std::uint8_t, 0x20> build_id{};
    std::copy(
        kMhgu140BuildId.begin(),
        kMhgu140BuildId.end(),
        build_id.begin()
    );
    const auto* matched_profile = profile_for_process(
        kMhguTitleId,
        build_id.data(),
        build_id.size()
    );
    assert(matched_profile != nullptr);
    assert(
        profile_for_process(
            kMhxxTitleId,
            build_id.data(),
            build_id.size()
        ) == nullptr
    );
    build_id[0] ^= 0xFFU;
    assert(
        profile_for_process(
            kMhguTitleId,
            build_id.data(),
            build_id.size()
        ) == nullptr
    );
    build_id[0] ^= 0xFFU;

    auto profile = *matched_profile;
    profile.scan_start_from_heap = 0x100;
    profile.scan_end_from_heap = 0x1000;

    constexpr std::uint64_t kList = 0x200;
    constexpr std::uint32_t kMonster = 0x4000;
    FakeMemory memory(0x10000);
    const std::uint8_t one = 1;
    memory.store(kList, one);
    memory.store(kList + 1, one);
    memory.store(kList + profile.pointer_list.pointers, kMonster);
    memory.store(kList + profile.pointer_list.count, one);

    const std::uint8_t secondary = 0x44;
    const std::uint8_t current_location =
        profile.monster.current_location_value;
    const std::uint16_t primary = 0x2220;
    const std::uint32_t health = 4000;
    const std::uint32_t maximum_health = 5000;
    const float size = 1.0F;
    memory.store(
        kMonster + profile.monster.location_flag,
        current_location
    );
    memory.store(
        kMonster + profile.monster.secondary_identifier,
        secondary
    );
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

    MonsterReader reader(memory, profile, 0);
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
    assert(reader.apply_size(request, verified));
    assert(verified == rathian->gold_percent);

    core::GameSnapshot changed{};
    assert(reader.read_snapshot(kList, core::Locale::English, changed));
    assert(changed.monsters[0].size_percent == rathian->gold_percent);

    request.target_percent = rathian->legal_max_percent + 1;
    assert(!reader.apply_size(request, verified));
    core::GameSnapshot rejected{};
    assert(reader.read_snapshot(kList, core::Locale::English, rejected));
    assert(rejected.monsters[0].size_percent == rathian->gold_percent);

    const std::uint8_t inactive_location = 0x44;
    memory.store(
        kMonster + profile.monster.location_flag,
        inactive_location
    );
    core::GameSnapshot inactive{};
    assert(reader.read_snapshot(kList, core::Locale::English, inactive));
    assert(inactive.monster_count == 0);
    request.target_percent = rathian->mini_percent;
    assert(!reader.apply_size(request, verified));

    std::cout << "switch adapter tests passed\n";
}
