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

constexpr MonsterDamagePatch kMonsterDamagePatch{
  0x00098BB0,
  0xE1A00006,
  0xE2860001,
};

constexpr ItemPouchLayout kItemPouchLayout{
  0x10D6920C,
  4,
  10,
  1,
  99,
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

constexpr MainWordPatchSet kAlchemyGaugeFullPatches{
  {{{0x0029E980, 0xE3440800}, {0x0029E984, 0xE5860000}}},
  2,
};

constexpr MainWordPatchSet kSpStatusNoExpirePatches{
  {{{0x0029E5AC, 0xE3A00000}}},
  1,
};

constexpr MainWordPatchSet kBowgunAutoReloadPatches{
  {{{0x002FE3A8, 0xE1C120B2}}},
  1,
};

constexpr MainWordPatchSet kConsumableItemsNoDecreasePatches{
  {{{0x003015F8, 0xE3A07000}}},
  1,
};

constexpr MainWordPatchSet kWeaponTransmogPatches{
  {{{0x000DAEE0, 0xE3A00001},
    {0x000DAEFC, 0x33000000},
    {0x000DDB5C, 0xEB01853F},
    {0x00283710, 0xEBFAEE52},
    {0x002874EC, 0xEBFADEDB},
    {0x00140C20, 0xE3520015},
    {0x00140C44, 0xE2411016},
    {0x00140C48, 0xE3510002}}},
  8,
};

constexpr MainWordPatchSet kArmorTransmogPatches{
  {{{0x00140E9C, 0xE3866000}, {0x00140F50, 0x13866000}}},
  2,
};

constexpr MainWordPatchSet kPalicoHealthNoDecreasePatches{
  {{{0x013F1E30, 0xE0804001},
    {0x013F1E34, 0xE1D451B0},
    {0x013F1E38, 0xE1C450BE},
    {0x013F1E3C, 0xE7901001},
    {0x013F1E40, 0xE12FFF1E},
    {0x0025903C, 0xEB46637B}}},
  6,
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
    kAlchemyGaugeFullPatches,
    kSpStatusNoExpirePatches,
    kBowgunAutoReloadPatches,
    kConsumableItemsNoDecreasePatches,
    kWeaponTransmogPatches,
    kArmorTransmogPatches,
    kPalicoHealthNoDecreasePatches,
  };

constexpr NumericWordPatchSet kHunterAffinityPatches{
  {{{0x000E400C,
     0xE3A00000,
     NumericWordEncoding::LinearImmediate,
     2,
     0}}},
  1,
  0,
  100,
};

constexpr NumericWordPatchSet kPalicoAffinityPatches{
  {{{0x000E5C24,
     0xE3A00000,
     NumericWordEncoding::LinearImmediate,
     2,
     0},
    {0x000E5C38,
     0xE3A00000,
     NumericWordEncoding::LinearImmediate,
     2,
     0}}},
  2,
  0,
  100,
};

constexpr NumericWordPatchSet kSpLevelPatches{
  {{{0x002AC5AC,
     0xE3A02000,
     NumericWordEncoding::LinearImmediate,
     1,
     -1}}},
  1,
  1,
  4,
};

constexpr NumericWordPatchSet kLongSwordSpiritGaugePatches{
  {{{0x002A31C0,
     0xE3A01000,
     NumericWordEncoding::LinearImmediate,
     1,
     0},
    {0x002EC6FC,
     0xE3A00001,
     NumericWordEncoding::Fixed,
     0,
     0}}},
  2,
  0,
  100,
};

constexpr NumericWordPatchSet kAttackMultiplierPatches{
  {{{0x013F1E50, 0xE1DF11B4, NumericWordEncoding::Fixed, 0, 0},
    {0x013F1E54, 0xE0000190, NumericWordEncoding::Fixed, 0, 0},
    {0x013F1E58, 0xE3500C7F, NumericWordEncoding::Fixed, 0, 0},
    {0x013F1E5C, 0xC3A00C7F, NumericWordEncoding::Fixed, 0, 0},
    {0x013F1E60, 0xE1C400B0, NumericWordEncoding::Fixed, 0, 0},
    {0x013F1E64, 0xE1A00009, NumericWordEncoding::Fixed, 0, 0},
    {0x013F1E68, 0xE12FFF1E, NumericWordEncoding::Fixed, 0, 0},
    {0x013F1E6C, 0x00000000, NumericWordEncoding::LinearWord, 1, 0},
    {0x000E2C38, 0xEB4C3C84, NumericWordEncoding::Fixed, 0, 0}}},
  9,
  1,
  10,
};

constexpr NumericWordPatchSet kDefenseMultiplierPatches{
  {{{0x013F1E70, 0xE1DF11B4, NumericWordEncoding::Fixed, 0, 0},
    {0x013F1E74, 0xE0000190, NumericWordEncoding::Fixed, 0, 0},
    {0x013F1E78, 0xE3500C7F, NumericWordEncoding::Fixed, 0, 0},
    {0x013F1E7C, 0xC3A00C7F, NumericWordEncoding::Fixed, 0, 0},
    {0x013F1E80, 0xE1C500B0, NumericWordEncoding::Fixed, 0, 0},
    {0x013F1E84, 0xE1A00004, NumericWordEncoding::Fixed, 0, 0},
    {0x013F1E88, 0xE12FFF1E, NumericWordEncoding::Fixed, 0, 0},
    {0x013F1E8C, 0x00000000, NumericWordEncoding::LinearWord, 1, 0},
    {0x000E3680, 0xEB4C39FA, NumericWordEncoding::Fixed, 0, 0}}},
  9,
  1,
  10,
};

constexpr NumericWordPatchSet kMovementSpeedMultiplierPatches{
  {{{0x013F1E00, 0xED9F0A01, NumericWordEncoding::Fixed, 0, 0},
    {0x013F1E04, 0xED800A00, NumericWordEncoding::Fixed, 0, 0},
    {0x013F1E08, 0xE12FFF1E, NumericWordEncoding::Fixed, 0, 0},
    {0x013F1E0C, 0x00000000, NumericWordEncoding::FloatTenths, 0, 0},
    {0x0029BF3C, 0xEB4557AF, NumericWordEncoding::Fixed, 0, 0}}},
  5,
  10,
  50,
};

constexpr NumericWordPatchSet kZennyPatches{
  {{{0x013F1E10,
     0xE3003000,
     NumericWordEncoding::ArmMovwImmediate,
     0,
     0},
    {0x013F1E14,
     0xE3403000,
     NumericWordEncoding::ArmMovtImmediate,
     0,
     0},
    {0x013F1E18, 0xE5853024, NumericWordEncoding::Fixed, 0, 0},
    {0x013F1E1C, 0xE12FFF1E, NumericWordEncoding::Fixed, 0, 0},
    {0x0062E500, 0xEB370E42, NumericWordEncoding::Fixed, 0, 0}}},
  5,
  0,
  9999999,
};

constexpr NumericWordPatchSet kWycademyPointsPatches{
  {{{0x013F1E20,
     0xE3003000,
     NumericWordEncoding::ArmMovwImmediate,
     0,
     0},
    {0x013F1E24,
     0xE3403000,
     NumericWordEncoding::ArmMovtImmediate,
     0,
     0},
    {0x013F1E28, 0xE585302C, NumericWordEncoding::Fixed, 0, 0},
    {0x013F1E2C, 0xE12FFF1E, NumericWordEncoding::Fixed, 0, 0},
    {0x0062E530, 0xEB370E3A, NumericWordEncoding::Fixed, 0, 0}}},
  5,
  0,
  9999999,
};

constexpr std::array<NumericWordPatchSet, core::kNumericFeatureCount>
  kNumericPatches{
    kHunterAffinityPatches,
    kPalicoAffinityPatches,
    kSpLevelPatches,
    kLongSwordSpiritGaugePatches,
    kAttackMultiplierPatches,
    kDefenseMultiplierPatches,
    kMovementSpeedMultiplierPatches,
    kZennyPatches,
    kWycademyPointsPatches,
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
    kMonsterDamagePatch,
    kItemPouchLayout,
    kRuntimePatches,
    kNumericPatches,
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
