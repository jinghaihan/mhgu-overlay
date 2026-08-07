#include "mhgu/platform/switch/game_profile.hpp"

#include <algorithm>

namespace mhgu::platform::switch_adapter {
namespace {

constexpr MonsterLayout kMonsterLayout{
  0x000D,
  0x4C,
  0x44,
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

constexpr FrameRatePatch kFrameRatePatch{
  0x018A6210,
  0x0000243C,
  0x41F00000,
  0x42700000,
};

constexpr MainWordPatchSet kMapAndLargeMonstersPatches{
  {{{0x0061BAD0, 0xE1A00000}, {0x0061CC94, 0xE3A00001}}},
  2,
};

constexpr MainWordPatchSet kCarryItemsIntoPouchPatches{
  {{{0x001943CC, 0x33000000}}},
  1,
};

constexpr MainWordPatchSet kInvinciblePatches{
  {{{0x0016B2A4, 0xE3A00000}}},
  1,
};

constexpr MainWordPatchSet kHealthNoDecreasePatches{
  {{{0x002F0CFC, 0xE302170F}}},
  1,
};

constexpr MainWordPatchSet kStaminaNoDecreasePatches{
  {{{0x002A3EC4, 0xE3A00001}}},
  1,
};

constexpr MainWordPatchSet kSharpnessNoDecreasePatches{
  {{{0x002AD3E0, 0xE6BF1070}}},
  1,
};

constexpr MainWordPatchSet kUnlockHunterArtSlotsPatches{
  {{{0x002778C0, 0xE3A00003}}},
  1,
};

constexpr MainWordPatchSet kUnlimitedHunterArtsPatches{
  {{{0x002A26E8, 0xE18020B3}, {0x002A26EC, 0xE1500000}}},
  2,
};

constexpr MainWordPatchSet kValorGaugeNoDecreasePatches{
  {{{0x00639C68, 0xEEBD0AC1},
    {0x00639C6C, 0xED800A00},
    {0x00639C70, 0xED841A18}}},
  3,
};

constexpr std::array<MainWordPatchSet, core::kRuntimeFeatureCount>
  kRuntimePatches{
    kMapAndLargeMonstersPatches,
    kCarryItemsIntoPouchPatches,
    kInvinciblePatches,
    kHealthNoDecreasePatches,
    kStaminaNoDecreasePatches,
    kSharpnessNoDecreasePatches,
    kUnlockHunterArtSlotsPatches,
    kUnlimitedHunterArtsPatches,
    kValorGaugeNoDecreasePatches,
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
    kFrameRatePatch,
    kRuntimePatches,
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
    if (profile.title_id == title_id && std::equal(
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
