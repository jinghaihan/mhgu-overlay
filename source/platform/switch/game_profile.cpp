#include "mhgu/platform/switch/game_profile.hpp"

#include <algorithm>

namespace mhgu::platform::switch_adapter {
namespace {

constexpr MonsterLayout kMonsterLayout{
    0x000D,
    0x4C,
    0x15EA,
    0x15F0,
    0x17B0,
    0x17B4,
    0x7628,
};

constexpr PointerListLayout kPointerListLayout{
    0x00,
    0x02,
    0x18,
    0x40,
    0x41,
};

constexpr GameProfile kProfiles[]{
    {
        "MHGU 1.4.0",
        core::GameId::Mhgu,
        kMhguTitleId,
        kMhgu140BuildId,
        0x10A00000,
        0x10F00000,
        kMonsterLayout,
        kPointerListLayout,
    },
};

}  // namespace

const GameProfile* profile_for_process(
    const std::uint64_t title_id,
    const std::uint8_t* build_id,
    const std::size_t build_id_size
) {
    if (build_id == nullptr || build_id_size < kBuildIdPrefixSize) {
        return nullptr;
    }
    for (const auto& profile : kProfiles) {
        if (profile.title_id == title_id &&
            std::equal(
                profile.build_id_prefix.begin(),
                profile.build_id_prefix.end(),
                build_id
            )) {
            return &profile;
        }
    }
    return nullptr;
}

}  // namespace mhgu::platform::switch_adapter
