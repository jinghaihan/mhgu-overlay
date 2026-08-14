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
  0,
  0,
  0,
  0x018A6210,
  0x0000243C,
  0x41F00000,
  0x42700000,
};

constexpr FrameRatePatch kMhxxFrameRatePatch{
  // MHXX's frame-rate mode selector uses a 32-bit pointer load and a byte
  // write before the float target is changed.
  0x00DFD9CC,
  0x000008B4,
  2,
  0x018AD81C,
  0x0000003C,
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

constexpr QuestLayout kQuestLayout{
  0x0188AD90,
  0x001C,
  0x44610000,
  0x00C4,
  // Atmosphere's second 78000000 offset is cumulative: C4 + 15A6.
  0x166A,
  0x00EC,
  0x29,
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

constexpr MainWordPatchSet kMhxxInvinciblePatches{
  {{{0x001661D8, 0xE12FFF1E}}},
  1,
};

constexpr MainWordPatchSet kMhxxMapAndLargeMonstersPatches{
  {{{0x00612F40, 0xE3A00001}, {0x00613FB4, 0xE3A00001}}},
  2,
};

constexpr MainWordPatchSet kMhxxCarryItemsIntoPouchPatches{
  {{{0x0019198C, 0x33000000}}},
  1,
};

constexpr MainWordPatchSet kMhxxHealthNoDecreasePatches{
  {{{0x002EDC5C, 0xE302170F}}},
  1,
};

constexpr MainWordPatchSet kMhxxStaminaNoDecreasePatches{
  {{{0x002A0E24, 0xE3A00001}, {0x002A2990, 0xE3A00001}}},
  2,
};

constexpr MainWordPatchSet kMhxxSharpnessNoDecreasePatches{
  {{{0x002AA340, 0xE6BF1070}}},
  1,
};

constexpr MainWordPatchSet kMhxxUnlockHunterArtSlotsPatches{
  {{{0x00274B14, 0xE3A00003}}},
  1,
};

constexpr MainWordPatchSet kMhxxUnlimitedHunterArtsPatches{
  {{{0x0029F648, 0xE18020B3}, {0x0029F64C, 0xE1500000}}},
  2,
};

constexpr MainWordPatchSet kMhxxValorGaugeNoDecreasePatches{
  {{{0x00299374, 0xE3A02040},
    {0x0029B650, 0xE3A00FFA},
    {0x0029B654, 0xE1C600B0}}},
  3,
};

constexpr MainWordPatchSet kMhxxAlchemyGaugeFullPatches{
  {{{0x0029B8E0, 0xE3440800}, {0x0029B8E4, 0xE5860000}}},
  2,
};

constexpr MainWordPatchSet kMhxxSpStatusNoExpirePatches{
  {{{0x0029B50C, 0xE3A00000}}},
  1,
};

constexpr MainWordPatchSet kMhxxBowgunAutoReloadPatches{
  {{{0x002FB308, 0xE1C120B2}}},
  1,
};

constexpr MainWordPatchSet kMhxxConsumableItemsNoDecreasePatches{
  {{{0x002FE558, 0xE3A07000}}},
  1,
};

constexpr MainWordPatchSet kMhxxWeaponTransmogPatches{
  {{{0x000D9C1C, 0xE3A00001},
    {0x000D9C38, 0x33000000},
    {0x000DC898, 0xEB018417},
    {0x002806EC, 0xEBFAF482},
    {0x002844C8, 0xEBFAE50B},
    {0x0013EE34, 0xE3520015},
    {0x0013EE58, 0xE2411016},
    {0x0013EE5C, 0xE3510002}}},
  8,
};

constexpr MainWordPatchSet kMhxxArmorTransmogPatches{
  {{{0x0013F0B0, 0xE3866000}, {0x0013F164, 0x13866000}}},
  2,
};

constexpr MonsterDamagePatch kMhxxMonsterDamagePatch{
  0x00097ADC,
  0xE1A00006,
  0xE2860001,
};

constexpr QuestLayout kMhxxQuestLayout{
  0x018AC1C0,
  0x001C,
  0x44610000,
  0x00C4,
  // The second 78000000 offset is cumulative: C4 + 15A6.
  0x15A6,
  0x00EC,
  0x29,
};

constexpr MainWordPatchSet kNoRuntimePatches{{}, 0};

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

constexpr NumericWordPatchSet kMhxxZennyPatches{
  {{{0x013E6710,
     0xE3003000,
     NumericWordEncoding::ArmMovwImmediate,
     0,
     0},
    {0x013E6714,
     0xE3403000,
     NumericWordEncoding::ArmMovtImmediate,
     0,
     0},
    {0x013E6718, 0xE5853024, NumericWordEncoding::Fixed, 0, 0},
    {0x013E671C, 0xE12FFF1E, NumericWordEncoding::Fixed, 0, 0},
    {0x00625374, 0xEB3704E5, NumericWordEncoding::Fixed, 0, 0}}},
  5,
  0,
  9999999,
};

constexpr NumericWordPatchSet kMhxxHunterAffinityPatches{
  {{{0x000E2D48,
     0xE3A00000,
     NumericWordEncoding::LinearImmediate,
     1,
     0}}},
  1,
  0,
  100,
};

constexpr NumericWordPatchSet kMhxxPalicoAffinityPatches{
  {{{0x000E4960,
     0xE3A00000,
     NumericWordEncoding::LinearImmediate,
     1,
     0},
    {0x000E4974,
     0xE3A00000,
     NumericWordEncoding::LinearImmediate,
     1,
     0}}},
  2,
  0,
  100,
};

constexpr NumericWordPatchSet kMhxxSpLevelPatches{
  {{{0x002A950C,
     0xE3A02000,
     NumericWordEncoding::LinearImmediate,
     1,
     -1}}},
  1,
  1,
  4,
};

constexpr NumericWordPatchSet kMhxxAttackMultiplierPatches{
  {{{0x013E66A8, 0xE59F1018, NumericWordEncoding::Fixed, 0, 0},
    {0x013E66AC, 0xE0000190, NumericWordEncoding::Fixed, 0, 0},
    {0x013E66B0, 0xE6BF0070, NumericWordEncoding::Fixed, 0, 0},
    {0x013E66B4, 0xE3500C7D, NumericWordEncoding::Fixed, 0, 0},
    {0x013E66B8, 0xC3A00C7D, NumericWordEncoding::Fixed, 0, 0},
    {0x013E66BC, 0xE300133A, NumericWordEncoding::Fixed, 0, 0},
    {0x013E66C0, 0xE1A400B1, NumericWordEncoding::Fixed, 0, 0},
    {0x013E66C4, 0xEAB3ECAA, NumericWordEncoding::Fixed, 0, 0},
    {0x013E66C8, 0x00000000, NumericWordEncoding::LinearWord, 1, 0},
    {0x000E1960, 0xEB4C1350, NumericWordEncoding::Fixed, 0, 0}}},
  10,
  1,
  10,
};

constexpr NumericWordPatchSet kMhxxDefenseMultiplierPatches{
  {{{0x013E66CC, 0xE0855001, NumericWordEncoding::Fixed, 0, 0},
    {0x013E66D0, 0xE59F1014, NumericWordEncoding::Fixed, 0, 0},
    {0x013E66D4, 0xE0000190, NumericWordEncoding::Fixed, 0, 0},
    {0x013E66D8, 0xE6BF0070, NumericWordEncoding::Fixed, 0, 0},
    {0x013E66DC, 0xE3500C7D, NumericWordEncoding::Fixed, 0, 0},
    {0x013E66E0, 0xC3A00C7D, NumericWordEncoding::Fixed, 0, 0},
    {0x013E66E4, 0xE1C500B0, NumericWordEncoding::Fixed, 0, 0},
    {0x013E66E8, 0xE12FFF1E, NumericWordEncoding::Fixed, 0, 0},
    {0x013E66EC, 0x00000000, NumericWordEncoding::LinearWord, 1, 0},
    {0x000E23B8, 0xEB4C10C3, NumericWordEncoding::Fixed, 0, 0}}},
  10,
  1,
  10,
};

constexpr NumericWordPatchSet kMhxxWycademyPointsPatches{
  {{{0x013E6720,
     0xE3003000,
     NumericWordEncoding::ArmMovwImmediate,
     0,
     0},
    {0x013E6724,
     0xE3403000,
     NumericWordEncoding::ArmMovtImmediate,
     0,
     0},
    {0x013E6728, 0xE585302C, NumericWordEncoding::Fixed, 0, 0},
    {0x013E672C, 0xE12FFF1E, NumericWordEncoding::Fixed, 0, 0},
    {0x006253A4, 0xEB3704DD, NumericWordEncoding::Fixed, 0, 0}}},
  5,
  0,
  9999999,
};

constexpr NumericWordPatchSet kNoNumericPatches{{}, 0, 0, 0};

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

constexpr std::array<MainWordPatchSet, core::kRuntimeFeatureCount>
  kMhxxRuntimePatches{
    kMhxxMapAndLargeMonstersPatches,
    kMhxxCarryItemsIntoPouchPatches,
    kMhxxInvinciblePatches,
    kMhxxHealthNoDecreasePatches,
    kMhxxStaminaNoDecreasePatches,
    kMhxxSharpnessNoDecreasePatches,
    kMhxxUnlockHunterArtSlotsPatches,
    kMhxxUnlimitedHunterArtsPatches,
    kMhxxValorGaugeNoDecreasePatches,
    kMhxxAlchemyGaugeFullPatches,
    kMhxxSpStatusNoExpirePatches,
    kMhxxBowgunAutoReloadPatches,
    kMhxxConsumableItemsNoDecreasePatches,
    kMhxxWeaponTransmogPatches,
    kMhxxArmorTransmogPatches,
    kNoRuntimePatches,
  };

constexpr std::array<NumericWordPatchSet, core::kNumericFeatureCount>
  kMhxxNumericPatches{
    kMhxxHunterAffinityPatches,
    kMhxxPalicoAffinityPatches,
    kMhxxSpLevelPatches,
    kNoNumericPatches,
    kMhxxAttackMultiplierPatches,
    kMhxxDefenseMultiplierPatches,
    kNoNumericPatches,
    kMhxxZennyPatches,
    kMhxxWycademyPointsPatches,
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
    kQuestLayout,
    kRuntimePatches,
    kNumericPatches,
  },
  {
    // The monster HP, maximum HP, and size fields match the current MHGU
    // object layout. The pointer-list range is an initial candidate and must
    // still be checked on a real MHXX process.
    "MHXX 1.5.1",
    core::GameId::Mhxx,
    kMhxxTitleId,
    kMhxx151BuildId,
    0x10A00000,
    0x10F00000,
    kMonsterLayout,
    kPointerListLayout,
    kMhxxFrameRatePatch,
    kMhxxMonsterDamagePatch,
    {},
    kMhxxQuestLayout,
    kMhxxRuntimePatches,
    kMhxxNumericPatches,
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
