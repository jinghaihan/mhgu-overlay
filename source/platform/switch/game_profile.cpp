#include "mhgu/platform/switch/game_profile.hpp"

#include <algorithm>

#include "mhgu/platform/switch/mhgu_profile.hpp"
#include "mhgu/platform/switch/mhxx_profile.hpp"

namespace mhgu::platform::switch_adapter {
namespace {

const GameProfile* const kProfiles[]{
  &mhgu_profile(),
  &mhxx_profile(),
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
  for (const auto* profile : kProfiles) {
    if (profile->title_id == title_id && std::equal(
                                          profile->build_id_prefix.begin(),
                                          profile->build_id_prefix.end(),
                                          build_id
                                        )) {
      return profile;
    }
  }
  return nullptr;
}

}  // namespace mhgu::platform::switch_adapter
