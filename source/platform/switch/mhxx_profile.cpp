#include "mhgu/platform/switch/mhxx_profile.hpp"

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
  0x00097ADC,
  0xE1A00006,
  0xE2860001,
};

constexpr QuestLayout kQuestLayout{
  0x018AC1C0,
  0x001C,
  0x44610000,
  0x00C4,
  // The second 78000000 offset is cumulative: C4 + 15A6.
  0x15A6,
  0x00EC,
  0x29,
};

constexpr MainWordPatchSet kInvinciblePatches{
  // This is the user-supplied MHXX invincibility patch, not the similarly
  // named alternative found in the reference archive.
  {{{0x001661D8, 0xE12FFF1E}}},
  1,
};

constexpr MainWordPatchSet kMapAndLargeMonstersPatches{
  {{{0x00612F40, 0xE3A00001}, {0x00613FB4, 0xE3A00001}}},
  2,
};

constexpr MainWordPatchSet kCarryItemsIntoPouchPatches{
  {{{0x0019198C, 0x33000000}}},
  1,
};

constexpr MainWordPatchSet kHealthNoDecreasePatches{
  {{{0x002EDC5C, 0xE302170F}}},
  1,
};

constexpr MainWordPatchSet kStaminaNoDecreasePatches{
  {{{0x002A0E24, 0xE3A00001}, {0x002A2990, 0xE3A00001}}},
  2,
};

constexpr MainWordPatchSet kSharpnessNoDecreasePatches{
  {{{0x002AA340, 0xE6BF1070}}},
  1,
};

constexpr MainWordPatchSet kUnlockHunterArtSlotsPatches{
  {{{0x00274B14, 0xE3A00003}}},
  1,
};

constexpr MainWordPatchSet kUnlimitedHunterArtsPatches{
  {{{0x0029F648, 0xE18020B3}, {0x0029F64C, 0xE1500000}}},
  2,
};

constexpr MainWordPatchSet kValorGaugeNoDecreasePatches{
  {{{0x00299374, 0xE3A02040},
    {0x0029B650, 0xE3A00FFA},
    {0x0029B654, 0xE1C600B0}}},
  3,
};

constexpr MainWordPatchSet kAlchemyGaugeFullPatches{
  {{{0x0029B8E0, 0xE3440800}, {0x0029B8E4, 0xE5860000}}},
  2,
};

constexpr MainWordPatchSet kSpStatusNoExpirePatches{
  {{{0x0029B50C, 0xE3A00000}}},
  1,
};

constexpr MainWordPatchSet kBowgunAutoReloadPatches{
  {{{0x002FB308, 0xE1C120B2}}},
  1,
};

constexpr MainWordPatchSet kConsumableItemsNoDecreasePatches{
  {{{0x002FE558, 0xE3A07000}}},
  1,
};

constexpr MainWordPatchSet kWeaponTransmogPatches{
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

constexpr MainWordPatchSet kArmorTransmogPatches{
  {{{0x0013F0B0, 0xE3866000}, {0x0013F164, 0x13866000}}},
  2,
};

constexpr MainWordPatchSet kNoRuntimePatches{{}, 0};

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
    kNoRuntimePatches,
  };

constexpr NumericWordPatchSet kHunterAffinityPatches{
  {{{0x000E2D48,
     0xE3A00000,
     NumericWordEncoding::LinearImmediate,
     1,
     0}}},
  1,
  0,
  100,
};

constexpr NumericWordPatchSet kPalicoAffinityPatches{
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

constexpr NumericWordPatchSet kSpLevelPatches{
  {{{0x002A950C,
     0xE3A02000,
     NumericWordEncoding::LinearImmediate,
     1,
     -1}}},
  1,
  1,
  4,
};

constexpr NumericWordPatchSet kAttackMultiplierPatches{
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

constexpr NumericWordPatchSet kDefenseMultiplierPatches{
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

constexpr NumericWordPatchSet kNoNumericPatches{{}, 0, 0, 0};

constexpr NumericWordPatchSet kZennyPatches{
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

constexpr NumericWordPatchSet kWycademyPointsPatches{
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

constexpr std::array<NumericWordPatchSet, core::kNumericFeatureCount>
  kNumericPatches{
    kHunterAffinityPatches,
    kPalicoAffinityPatches,
    kSpLevelPatches,
    kNoNumericPatches,
    kAttackMultiplierPatches,
    kDefenseMultiplierPatches,
    kNoNumericPatches,
    kZennyPatches,
    kWycademyPointsPatches,
  };

constexpr GameProfile kProfile{
  // The monster fields match the current MHGU object layout. The pointer-list
  // range is an initial candidate and must still be checked on real hardware.
  "MHXX 1.5.1",
  core::GameId::Mhxx,
  kMhxxTitleId,
  kMhxx151BuildId,
  0x10A00000,
  0x10F00000,
  kMonsterLayout,
  kPointerListLayout,
  kFrameRatePatch,
  kMonsterDamagePatch,
  {},
  kQuestLayout,
  kRuntimePatches,
  kNumericPatches,
};

}  // namespace

const GameProfile& mhxx_profile() {
  return kProfile;
}

}  // namespace mhgu::platform::switch_adapter
