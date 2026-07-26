#include "mhgu/platform/switch/game_profile.hpp"

namespace mhgu::platform::switch_adapter {
namespace {

constexpr MonsterLayout kMonsterLayout{
    0x000D,
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
        0x10A00000,
        0x10F00000,
        kMonsterLayout,
        kPointerListLayout,
    },
    {
        "MHXX Switch",
        core::GameId::Mhxx,
        kMhxxTitleId,
        0x10A00000,
        0x10F00000,
        kMonsterLayout,
        kPointerListLayout,
    },
};

}  // namespace

const GameProfile* profile_for_title(const std::uint64_t title_id) {
    for (const auto& profile : kProfiles) {
        if (profile.title_id == title_id) {
            return &profile;
        }
    }
    return nullptr;
}

}  // namespace mhgu::platform::switch_adapter
