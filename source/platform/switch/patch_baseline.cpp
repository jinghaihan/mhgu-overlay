#include "mhgu/platform/switch/patch_baseline.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>

#ifdef __SWITCH__
#include <sys/stat.h>
#endif

namespace mhgu::platform::switch_adapter {
namespace {

constexpr std::array<std::uint8_t, 8> kMagic{
  'M', 'H', 'G', 'U', 'P', 'B', '0', '1',
};

void append_u32(std::vector<std::uint8_t>& bytes, const std::uint32_t value) {
  for (std::size_t index = 0; index < sizeof(value); ++index) {
    bytes.push_back(static_cast<std::uint8_t>(value >> (index * 8)));
  }
}

void append_u64(std::vector<std::uint8_t>& bytes, const std::uint64_t value) {
  for (std::size_t index = 0; index < sizeof(value); ++index) {
    bytes.push_back(static_cast<std::uint8_t>(value >> (index * 8)));
  }
}

bool read_u32(
  const std::vector<std::uint8_t>& bytes,
  std::size_t& cursor,
  std::uint32_t& value
) {
  if (cursor > bytes.size() || sizeof(value) > bytes.size() - cursor) {
    return false;
  }
  value = 0;
  for (std::size_t index = 0; index < sizeof(value); ++index) {
    value |= static_cast<std::uint32_t>(bytes[cursor++]) << (index * 8);
  }
  return true;
}

bool read_u64(
  const std::vector<std::uint8_t>& bytes,
  std::size_t& cursor,
  std::uint64_t& value
) {
  if (cursor > bytes.size() || sizeof(value) > bytes.size() - cursor) {
    return false;
  }
  value = 0;
  for (std::size_t index = 0; index < sizeof(value); ++index) {
    value |= static_cast<std::uint64_t>(bytes[cursor++]) << (index * 8);
  }
  return true;
}

std::uint32_t checksum(const std::uint8_t* bytes, const std::size_t size) {
  std::uint32_t value = 2166136261U;
  for (std::size_t index = 0; index < size; ++index) {
    value ^= bytes[index];
    value *= 16777619U;
  }
  return value;
}

void append_patch_set_offsets(
  const MainWordPatchSet& patch_set,
  std::array<std::uint64_t, kMaxPatchBaselineEntries>& offsets,
  std::size_t& count
) {
  if (patch_set.count > patch_set.patches.size()) {
    count = offsets.size() + 1;
    return;
  }
  for (std::size_t index = 0; index < patch_set.count; ++index) {
    if (patch_set.patches[index].offset == 0) {
      continue;
    }
    if (count >= offsets.size()) {
      count = offsets.size() + 1;
      return;
    }
    offsets[count++] = patch_set.patches[index].offset;
  }
}

}  // namespace

const PatchBaselineEntry* PatchBaseline::find(
  const std::uint64_t offset
) const {
  const auto begin = entries.begin();
  const auto end = begin + static_cast<std::ptrdiff_t>(count);
  const auto match = std::lower_bound(
    begin,
    end,
    offset,
    [](const PatchBaselineEntry& entry, const std::uint64_t candidate) {
      return entry.offset < candidate;
    }
  );
  return match != end && match->offset == offset ? &*match : nullptr;
}

std::size_t collect_patch_offsets(
  const GameProfile& profile,
  std::array<std::uint64_t, kMaxPatchBaselineEntries>& offsets
) {
  std::size_t count{};
  if (profile.monster_damage.offset != 0) {
    offsets[count++] = profile.monster_damage.offset;
  }
  for (const auto& patch_set : profile.runtime_patches) {
    append_patch_set_offsets(patch_set, offsets, count);
  }
  for (const auto& patch_set : profile.numeric_patches) {
    if (patch_set.count > patch_set.patches.size()) {
      return 0;
    }
    for (std::size_t index = 0; index < patch_set.count; ++index) {
      if (patch_set.patches[index].offset == 0) {
        continue;
      }
      if (count >= offsets.size()) {
        return 0;
      }
      offsets[count++] = patch_set.patches[index].offset;
    }
  }
  if (count > offsets.size()) {
    return 0;
  }
  std::sort(offsets.begin(), offsets.begin() + static_cast<std::ptrdiff_t>(count));
  const auto unique_end = std::unique(
    offsets.begin(), offsets.begin() + static_cast<std::ptrdiff_t>(count)
  );
  return static_cast<std::size_t>(unique_end - offsets.begin());
}

bool patch_baseline_matches_profile(
  const PatchBaseline& baseline, const GameProfile& profile
) {
  if (baseline.title_id != profile.title_id ||
      baseline.build_id_prefix != profile.build_id_prefix ||
      baseline.count == 0 || baseline.count > baseline.entries.size()) {
    return false;
  }
  std::array<std::uint64_t, kMaxPatchBaselineEntries> offsets{};
  const auto count = collect_patch_offsets(profile, offsets);
  if (count == 0 || count != baseline.count) {
    return false;
  }
  for (std::size_t index = 0; index < count; ++index) {
    if (baseline.entries[index].offset != offsets[index]) {
      return false;
    }
  }
  return true;
}

PatchBaselineStore::PatchBaselineStore(std::string path)
  : path_(std::move(path)) {}

bool PatchBaselineStore::load(
  const GameProfile& profile, PatchBaseline& baseline
) const {
  auto* file = std::fopen(path_.c_str(), "rb");
  if (file == nullptr) {
    return false;
  }
  std::vector<std::uint8_t> bytes;
  std::array<std::uint8_t, 512> chunk{};
  while (true) {
    const auto read = std::fread(chunk.data(), 1, chunk.size(), file);
    bytes.insert(bytes.end(), chunk.begin(), chunk.begin() + read);
    if (read != chunk.size()) {
      break;
    }
  }
  const auto read_failed = std::ferror(file) != 0;
  std::fclose(file);
  constexpr std::size_t kMinimumSize =
    kMagic.size() + sizeof(std::uint64_t) + kBuildIdPrefixSize +
    sizeof(std::uint32_t) + sizeof(std::uint32_t);
  if (read_failed || bytes.size() < kMinimumSize ||
      !std::equal(kMagic.begin(), kMagic.end(), bytes.begin())) {
    return false;
  }
  const auto stored_checksum_offset = bytes.size() - sizeof(std::uint32_t);
  std::size_t checksum_cursor = stored_checksum_offset;
  std::uint32_t stored_checksum{};
  if (!read_u32(bytes, checksum_cursor, stored_checksum) ||
      stored_checksum != checksum(bytes.data(), stored_checksum_offset)) {
    return false;
  }

  PatchBaseline loaded{};
  std::size_t cursor = kMagic.size();
  if (!read_u64(bytes, cursor, loaded.title_id) ||
      cursor + loaded.build_id_prefix.size() > stored_checksum_offset) {
    return false;
  }
  std::copy_n(
    bytes.begin() + static_cast<std::ptrdiff_t>(cursor),
    loaded.build_id_prefix.size(),
    loaded.build_id_prefix.begin()
  );
  cursor += loaded.build_id_prefix.size();
  std::uint32_t count{};
  if (!read_u32(bytes, cursor, count) || count == 0 ||
      count > loaded.entries.size() ||
      stored_checksum_offset - cursor != count * 12U) {
    return false;
  }
  loaded.count = count;
  for (std::size_t index = 0; index < loaded.count; ++index) {
    if (!read_u64(bytes, cursor, loaded.entries[index].offset) ||
        !read_u32(bytes, cursor, loaded.entries[index].value) ||
        (index > 0 && loaded.entries[index - 1].offset >=
                        loaded.entries[index].offset)) {
      return false;
    }
  }
  if (!patch_baseline_matches_profile(loaded, profile)) {
    return false;
  }
  baseline = loaded;
  return true;
}

bool PatchBaselineStore::save(
  const GameProfile& profile, const PatchBaseline& baseline
) const {
  if (!patch_baseline_matches_profile(baseline, profile)) {
    return false;
  }
#ifdef __SWITCH__
  mkdir("sdmc:/config", 0777);
  mkdir("sdmc:/config/mhgu-overlay", 0777);
#endif
  std::vector<std::uint8_t> bytes(kMagic.begin(), kMagic.end());
  append_u64(bytes, baseline.title_id);
  bytes.insert(
    bytes.end(),
    baseline.build_id_prefix.begin(),
    baseline.build_id_prefix.end()
  );
  append_u32(bytes, static_cast<std::uint32_t>(baseline.count));
  for (std::size_t index = 0; index < baseline.count; ++index) {
    append_u64(bytes, baseline.entries[index].offset);
    append_u32(bytes, baseline.entries[index].value);
  }
  append_u32(bytes, checksum(bytes.data(), bytes.size()));

  const auto temporary_path = path_ + ".tmp";
  auto* file = std::fopen(temporary_path.c_str(), "wb");
  if (file == nullptr) {
    return false;
  }
  const auto written = std::fwrite(bytes.data(), 1, bytes.size(), file);
  const auto flushed = std::fflush(file) == 0;
  const auto closed = std::fclose(file) == 0;
  if (written != bytes.size() || !flushed || !closed) {
    std::remove(temporary_path.c_str());
    return false;
  }
  if (std::rename(temporary_path.c_str(), path_.c_str()) != 0) {
    std::remove(temporary_path.c_str());
    return false;
  }
  return true;
}

}  // namespace mhgu::platform::switch_adapter
