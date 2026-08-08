#include "mhgu/platform/switch/resource_scanner.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>
#include <utility>

#ifdef __SWITCH__
#include <sys/stat.h>
#endif

namespace mhgu::platform::switch_adapter {
namespace {

constexpr std::size_t kReadChunkSize = 64 * 1024;
constexpr std::size_t kFilterCandidatesPerAdvance = 64;
constexpr std::size_t kMaxStoredCandidates = 4096;

std::uint32_t read_u32(const std::uint8_t* bytes) {
  std::uint32_t value{};
  std::memcpy(&value, bytes, sizeof(value));
  return value;
}

}  // namespace

ResourceScanner::ResourceScanner(
  MemoryAccess& memory,
  const PlayerResourceLayout& layout,
  const std::uint64_t heap_base,
  const std::uint64_t heap_size,
  const std::uint64_t title_id,
  const char* profile_name,
  std::string report_path
)
  : memory_(memory),
    layout_(layout),
    structure_read_size_(
      std::max(layout.zenny, layout.wycademy_points) + sizeof(std::uint32_t)
    ),
    heap_base_(heap_base),
    heap_size_(heap_size),
    title_id_(title_id),
    profile_name_(profile_name),
    report_path_(std::move(report_path)),
    buffer_(kReadChunkSize + structure_read_size_ - 1) {}

bool ResourceScanner::start_initial(
  const std::uint32_t zenny, const std::uint32_t wycademy_points
) {
  return begin(ResourceScanMode::Initial, zenny, wycademy_points);
}

bool ResourceScanner::start_filter(
  const std::uint32_t zenny, const std::uint32_t wycademy_points
) {
  return begin(ResourceScanMode::Filter, zenny, wycademy_points);
}

bool ResourceScanner::begin(
  const ResourceScanMode mode,
  const std::uint32_t zenny,
  const std::uint32_t wycademy_points
) {
  if (active()) {
    return false;
  }
  ++view_.stage_number;
  view_.mode = mode;
  view_.expected_zenny = zenny;
  view_.expected_wycademy_points = wycademy_points;
  view_.progress = 0;
  view_.candidate_count = 0;
  view_.stored_candidate_count = 0;
  view_.skipped_read_count = 0;
  view_.preview_count = 0;
  view_.preview = {};
  if ((zenny == 0 && wycademy_points == 0) ||
      zenny > layout_.maximum_value ||
      wycademy_points > layout_.maximum_value ||
      layout_.zenny == layout_.wycademy_points ||
      structure_read_size_ > 0x1000 || heap_size_ == 0 ||
      heap_base_ > std::numeric_limits<std::uint64_t>::max() - heap_size_) {
    view_.status = ResourceScanStatus::InvalidInput;
    view_.progress_total = 0;
    return true;
  }

  if (mode == ResourceScanMode::Initial) {
    scan_offset_ = 0;
    candidates_.clear();
    view_.progress_total = heap_size_;
  } else {
    filter_index_ = 0;
    filtered_candidates_.clear();
    view_.progress_total = candidates_.size();
    if (candidates_.empty()) {
      view_.status = write_report() ? ResourceScanStatus::Complete
                                    : ResourceScanStatus::ReportFailed;
      return true;
    }
  }
  view_.status = ResourceScanStatus::Scanning;
  return true;
}

void ResourceScanner::advance(const std::size_t byte_budget) {
  if (!active()) {
    return;
  }
  if (view_.mode == ResourceScanMode::Initial) {
    advance_initial(byte_budget);
  } else {
    advance_filter();
  }
}

bool ResourceScanner::active() const {
  return view_.status == ResourceScanStatus::Scanning;
}

const ResourceScanView& ResourceScanner::view() const {
  return view_;
}

void ResourceScanner::advance_initial(std::size_t byte_budget) {
  if (byte_budget == 0) {
    return;
  }
  while (scan_offset_ < heap_size_ && byte_budget != 0) {
    const auto remaining = heap_size_ - scan_offset_;
    const auto body_size = static_cast<std::size_t>(std::min<std::uint64_t>(
      remaining, std::min<std::size_t>(kReadChunkSize, byte_budget)
    ));
    const auto read_size = static_cast<std::size_t>(std::min<std::uint64_t>(
      remaining, body_size + structure_read_size_ - 1
    ));
    const auto address = heap_base_ + scan_offset_;
    if (memory_.read(address, buffer_.data(), read_size)) {
      inspect_chunk(address, buffer_.data(), body_size, read_size);
    } else {
      ++view_.skipped_read_count;
    }
    scan_offset_ += body_size;
    view_.progress = scan_offset_;
    byte_budget -= body_size;
  }
  if (scan_offset_ >= heap_size_) {
    finish();
  }
}

void ResourceScanner::advance_filter() {
  const auto end = std::min(
    candidates_.size(), filter_index_ + kFilterCandidatesPerAdvance
  );
  constexpr std::size_t kMaximumReadSize = 0x1000;
  std::array<std::uint8_t, kMaximumReadSize> values{};
  const auto first_offset = std::min(layout_.zenny, layout_.wycademy_points);
  const auto read_size = structure_read_size_ - first_offset;
  for (; filter_index_ < end; ++filter_index_) {
    const auto& candidate = candidates_[filter_index_];
    if (!memory_.read(
          candidate.address + first_offset, values.data(), read_size
        )) {
      ++view_.skipped_read_count;
      continue;
    }
    const auto zenny =
      read_u32(values.data() + layout_.zenny - first_offset);
    const auto wycademy_points =
      read_u32(values.data() + layout_.wycademy_points - first_offset);
    if (zenny == view_.expected_zenny &&
        wycademy_points == view_.expected_wycademy_points) {
      filtered_candidates_.push_back({
        candidate.address,
        candidate.heap_offset,
        zenny,
        wycademy_points,
      });
    }
  }
  view_.progress = filter_index_;
  view_.candidate_count =
    static_cast<std::uint32_t>(filtered_candidates_.size());
  if (filter_index_ >= candidates_.size()) {
    candidates_.swap(filtered_candidates_);
    finish();
  }
}

void ResourceScanner::inspect_chunk(
  const std::uint64_t address,
  const std::uint8_t* bytes,
  const std::size_t body_size,
  const std::size_t read_size
) {
  for (std::size_t offset = 0;
       offset < body_size && offset + structure_read_size_ <= read_size;
       offset += 4) {
    const auto zenny = read_u32(bytes + offset + layout_.zenny);
    if (zenny != view_.expected_zenny) {
      continue;
    }
    const auto wycademy_points =
      read_u32(bytes + offset + layout_.wycademy_points);
    if (wycademy_points != view_.expected_wycademy_points) {
      continue;
    }
    append_candidate({
      address + offset,
      address + offset - heap_base_,
      zenny,
      wycademy_points,
    });
  }
}

void ResourceScanner::append_candidate(
  const ResourceAddressCandidate& candidate
) {
  ++view_.candidate_count;
  if (candidates_.size() < kMaxStoredCandidates) {
    candidates_.push_back(candidate);
  }
  if (view_.preview_count < view_.preview.size()) {
    view_.preview[view_.preview_count++] = candidate;
  }
}

void ResourceScanner::finish() {
  view_.stored_candidate_count =
    static_cast<std::uint32_t>(candidates_.size());
  view_.preview = {};
  view_.preview_count =
    std::min(candidates_.size(), view_.preview.size());
  for (std::size_t index = 0; index < view_.preview_count; ++index) {
    view_.preview[index] = candidates_[index];
  }
  view_.status = write_report() ? ResourceScanStatus::Complete
                                : ResourceScanStatus::ReportFailed;
}

bool ResourceScanner::write_report() {
  if (report_path_.empty()) {
    return true;
  }
#ifdef __SWITCH__
  mkdir("sdmc:/config", 0777);
  mkdir("sdmc:/config/mhgu-overlay", 0777);
#endif
  auto* file = std::fopen(
    report_path_.c_str(), report_initialized_ ? "a" : "w"
  );
  if (file == nullptr) {
    return false;
  }
  std::fprintf(file, "MHGU resource address diagnostic\n");
  std::fprintf(file, "stage=%u\n", view_.stage_number);
  std::fprintf(
    file,
    "mode=%s\n",
    view_.mode == ResourceScanMode::Initial ? "initial" : "filter"
  );
  std::fprintf(
    file, "profile=%s\n", profile_name_ == nullptr ? "unknown" : profile_name_
  );
  std::fprintf(
    file,
    "title_id=%016llX\n",
    static_cast<unsigned long long>(title_id_)
  );
  std::fprintf(
    file,
    "heap_base=%016llX\n",
    static_cast<unsigned long long>(heap_base_)
  );
  std::fprintf(
    file,
    "heap_size=%016llX\n",
    static_cast<unsigned long long>(heap_size_)
  );
  std::fprintf(file, "zenny=%u\n", view_.expected_zenny);
  std::fprintf(
    file, "wycademy_points=%u\n", view_.expected_wycademy_points
  );
  std::fprintf(file, "skipped_reads=%u\n", view_.skipped_read_count);
  std::fprintf(file, "candidate_count=%u\n", view_.candidate_count);
  std::fprintf(file, "stored_candidates=%zu\n", candidates_.size());
  for (const auto& candidate : candidates_) {
    std::fprintf(
      file,
      "candidate address=%016llX heap_offset=%016llX zenny=%u "
      "wycademy_points=%u\n",
      static_cast<unsigned long long>(candidate.address),
      static_cast<unsigned long long>(candidate.heap_offset),
      candidate.zenny,
      candidate.wycademy_points
    );
  }
  std::fprintf(file, "end_stage=%u\n\n", view_.stage_number);
  const auto success = std::fclose(file) == 0;
  if (success) {
    report_initialized_ = true;
  }
  return success;
}

}  // namespace mhgu::platform::switch_adapter
