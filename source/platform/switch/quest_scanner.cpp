#include "mhgu/platform/switch/quest_scanner.hpp"

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

constexpr std::uint32_t kQuestDataMagic = 0x434B0000U;
constexpr std::uint32_t kQuestDataCount = 1;
constexpr std::size_t kQuestHeaderSize = 0x18;
constexpr std::size_t kReadChunkSize = 64 * 1024;
constexpr std::size_t kMaxStoredCandidates = 4096;

std::uint32_t read_u32(const std::uint8_t* bytes) {
  std::uint32_t value{};
  std::memcpy(&value, bytes, sizeof(value));
  return value;
}

std::int32_t read_i32(const std::uint8_t* bytes) {
  std::int32_t value{};
  std::memcpy(&value, bytes, sizeof(value));
  return value;
}

bool valid_level(const std::uint8_t level) {
  return level <= 16 || (level >= 101 && level <= 116);
}

bool valid_enemy_level(const std::uint8_t level) {
  return level == 0 || level == 1 || level == 3 || level == 5;
}

bool valid_map(const std::uint8_t map) {
  return (map >= 1 && map <= 27) || (map >= 101 && map <= 104);
}

bool is_diagnostic_quest(const std::int32_t quest_id) {
  constexpr std::int32_t kQuestIds[]{
    10631, 10756, 10757, 10758, 10759, 10760, 10761, 11401, 11412,
    11413, 11414, 11415, 11416, 11417, 11458, 11459, 11460,
  };
  return std::find(std::begin(kQuestIds), std::end(kQuestIds), quest_id) !=
         std::end(kQuestIds);
}

bool decode_fields(
  const std::uint8_t* bytes,
  const std::uint64_t address,
  const std::uint64_t heap_base,
  const QuestDataLayout layout,
  const bool diagnostic_only,
  QuestDataCandidate& candidate
) {
  const auto index = read_i32(bytes);
  const auto quest_id = read_i32(bytes + 0x04);
  const auto type = bytes[0x08];
  const auto subtype = bytes[0x09];
  const auto level = bytes[0x0A];
  const auto enemy_level = bytes[0x0B];
  const auto map = bytes[0x0C];
  const auto start_type = bytes[0x0D];
  const auto quest_time = bytes[0x0E];
  const auto faints = bytes[0x0F];
  if (index < 0 || index > 4096 || quest_id < 10000 || quest_id > 19999 ||
      (diagnostic_only && !is_diagnostic_quest(quest_id)) ||
      type > 5 || subtype > 11 || !valid_level(level) ||
      !valid_enemy_level(enemy_level) || !valid_map(map) || start_type > 2 ||
      quest_time > 60 || faints > 9) {
    return false;
  }

  candidate = {
    address,
    address - heap_base,
    quest_id,
    layout,
    type,
    subtype,
    level,
    enemy_level,
    map,
    start_type,
    quest_time,
    faints,
  };
  return true;
}

bool decode_resource_candidate(
  const std::uint8_t* bytes,
  const std::uint64_t address,
  const std::uint64_t heap_base,
  QuestDataCandidate& candidate
) {
  return read_u32(bytes) == kQuestDataMagic &&
         read_u32(bytes + 0x04) == kQuestDataCount &&
         decode_fields(
           bytes + 0x08,
           address,
           heap_base,
           QuestDataLayout::Resource,
           false,
           candidate
         );
}

bool decode_runtime_candidate(
  const std::uint8_t* bytes,
  const std::uint64_t address,
  const std::uint64_t heap_base,
  QuestDataCandidate& candidate
) {
  return decode_fields(
    bytes,
    address,
    heap_base,
    QuestDataLayout::Runtime,
    true,
    candidate
  );
}

}  // namespace

QuestScanner::QuestScanner(
  MemoryAccess& memory,
  const std::uint64_t heap_base,
  const std::uint64_t heap_size,
  const std::uint64_t title_id,
  const char* profile_name,
  std::string report_path
)
  : memory_(memory),
    heap_base_(heap_base),
    heap_size_(heap_size),
    title_id_(title_id),
    profile_name_(profile_name),
    report_path_(std::move(report_path)),
    buffer_(kReadChunkSize + kQuestHeaderSize - 1) {}

bool QuestScanner::start() {
  if (active() || heap_size_ == 0 ||
      heap_base_ > std::numeric_limits<std::uint64_t>::max() - heap_size_) {
    return false;
  }
  ++view_.scan_number;
  view_.status = QuestScanStatus::Scanning;
  view_.scanned_bytes = 0;
  view_.total_bytes = heap_size_;
  view_.candidate_count = 0;
  view_.stored_candidate_count = 0;
  view_.skipped_read_count = 0;
  view_.preview_count = 0;
  view_.preview = {};
  scan_offset_ = 0;
  candidates_.clear();
  return true;
}

void QuestScanner::advance(std::size_t byte_budget) {
  if (!active() || byte_budget == 0) {
    return;
  }

  while (scan_offset_ < heap_size_ && byte_budget != 0) {
    const auto remaining = heap_size_ - scan_offset_;
    const auto body_size = static_cast<std::size_t>(std::min<std::uint64_t>(
      remaining, std::min<std::size_t>(kReadChunkSize, byte_budget)
    ));
    const auto read_size = static_cast<std::size_t>(std::min<std::uint64_t>(
      remaining, body_size + kQuestHeaderSize - 1
    ));
    const auto address = heap_base_ + scan_offset_;
    if (memory_.read(address, buffer_.data(), read_size)) {
      inspect_chunk(address, buffer_.data(), body_size, read_size);
    } else {
      ++view_.skipped_read_count;
    }
    scan_offset_ += body_size;
    view_.scanned_bytes = scan_offset_;
    byte_budget -= body_size;
  }

  if (scan_offset_ >= heap_size_) {
    finish();
  }
}

bool QuestScanner::active() const {
  return view_.status == QuestScanStatus::Scanning;
}

const QuestScanView& QuestScanner::view() const {
  return view_;
}

void QuestScanner::inspect_chunk(
  const std::uint64_t address,
  const std::uint8_t* bytes,
  const std::size_t body_size,
  const std::size_t read_size
) {
  for (std::size_t offset = 0;
       offset < body_size && offset + kQuestHeaderSize <= read_size;
       offset += 4) {
    QuestDataCandidate candidate{};
    const auto resource =
      bytes[offset + 2] == 0x4B && bytes[offset + 3] == 0x43 &&
      decode_resource_candidate(
        bytes + offset, address + offset, heap_base_, candidate
      );
    auto runtime = false;
    if (!resource) {
      runtime = decode_runtime_candidate(
        bytes + offset, address + offset, heap_base_, candidate
      );
      if (runtime) {
        const auto nested_resource = std::find_if(
          candidates_.begin(),
          candidates_.end(),
          [&candidate](const QuestDataCandidate& existing) {
            return existing.layout == QuestDataLayout::Resource &&
                   existing.address + 8 == candidate.address;
          }
        );
        runtime = nested_resource == candidates_.end();
      }
    }
    if (!resource && !runtime) {
      continue;
    }
    ++view_.candidate_count;
    if (candidates_.size() < kMaxStoredCandidates) {
      candidates_.push_back(candidate);
    }
    if (view_.preview_count < view_.preview.size()) {
      view_.preview[view_.preview_count++] = candidate;
    }
  }
}

void QuestScanner::finish() {
  view_.stored_candidate_count =
    static_cast<std::uint32_t>(candidates_.size());
  rebuild_preview();
  view_.status = write_report() ? QuestScanStatus::Complete
                                : QuestScanStatus::ReportFailed;
}

void QuestScanner::rebuild_preview() {
  view_.preview = {};
  view_.preview_count = 0;
  const auto append = [this](const QuestDataCandidate& candidate) {
    if (view_.preview_count < view_.preview.size()) {
      view_.preview[view_.preview_count++] = candidate;
    }
  };
  for (const auto& candidate : candidates_) {
    if (candidate.layout == QuestDataLayout::Runtime) {
      append(candidate);
    }
  }
  for (const auto& candidate : candidates_) {
    if (candidate.layout == QuestDataLayout::Resource &&
        is_diagnostic_quest(candidate.quest_id)) {
      append(candidate);
    }
  }
  for (const auto& candidate : candidates_) {
    if (!is_diagnostic_quest(candidate.quest_id)) {
      append(candidate);
    }
  }
}

bool QuestScanner::write_report() {
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
  std::fprintf(file, "MHGU QuestData diagnostic\n");
  std::fprintf(file, "scan=%u\n", view_.scan_number);
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
  std::fprintf(
    file,
    "scanned_bytes=%llu\n",
    static_cast<unsigned long long>(view_.scanned_bytes)
  );
  std::fprintf(file, "skipped_reads=%u\n", view_.skipped_read_count);
  std::fprintf(file, "candidate_count=%u\n", view_.candidate_count);
  std::fprintf(file, "stored_candidates=%zu\n", candidates_.size());
  for (const auto& candidate : candidates_) {
    std::fprintf(
      file,
      "candidate address=%016llX heap_offset=%016llX quest_id=%d layout=%s "
      "type=%u subtype=%u level=%u enemy_level=%u map=%u start=%u "
      "time=%u faints=%u\n",
      static_cast<unsigned long long>(candidate.address),
      static_cast<unsigned long long>(candidate.heap_offset),
      candidate.quest_id,
      candidate.layout == QuestDataLayout::Runtime ? "runtime" : "resource",
      candidate.type,
      candidate.subtype,
      candidate.level,
      candidate.enemy_level,
      candidate.map,
      candidate.start_type,
      candidate.quest_time,
      candidate.faints
    );
  }
  std::fprintf(file, "end_scan=%u\n\n", view_.scan_number);
  const auto success = std::fclose(file) == 0;
  if (success) {
    report_initialized_ = true;
  }
  return success;
}

}  // namespace mhgu::platform::switch_adapter
