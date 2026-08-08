#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "mhgu/platform/switch/memory.hpp"

namespace mhgu::platform::switch_adapter {

enum class QuestScanStatus : std::uint8_t {
  Idle,
  Scanning,
  Complete,
  ReportFailed,
};

enum class QuestDataLayout : std::uint8_t {
  Resource,
  Runtime,
};

struct QuestDataCandidate {
  std::uint64_t address{};
  std::uint64_t heap_offset{};
  std::int32_t quest_id{};
  QuestDataLayout layout{QuestDataLayout::Resource};
  std::uint8_t type{};
  std::uint8_t subtype{};
  std::uint8_t level{};
  std::uint8_t enemy_level{};
  std::uint8_t map{};
  std::uint8_t start_type{};
  std::uint8_t quest_time{};
  std::uint8_t faints{};
};

constexpr std::size_t kQuestScanPreviewCapacity = 6;

struct QuestScanView {
  QuestScanStatus status{QuestScanStatus::Idle};
  std::uint32_t scan_number{};
  std::uint64_t scanned_bytes{};
  std::uint64_t total_bytes{};
  std::uint32_t candidate_count{};
  std::uint32_t stored_candidate_count{};
  std::uint32_t skipped_read_count{};
  std::size_t preview_count{};
  std::array<QuestDataCandidate, kQuestScanPreviewCapacity> preview{};
};

class QuestScanner {
public:
  QuestScanner(
    MemoryAccess& memory,
    std::uint64_t heap_base,
    std::uint64_t heap_size,
    std::uint64_t title_id,
    const char* profile_name,
    std::string report_path
  );

  bool start();
  void advance(std::size_t byte_budget);
  bool active() const;
  const QuestScanView& view() const;

private:
  bool write_report();
  void finish();
  void rebuild_preview();
  void inspect_chunk(
    std::uint64_t address,
    const std::uint8_t* bytes,
    std::size_t body_size,
    std::size_t read_size
  );

  MemoryAccess& memory_;
  std::uint64_t heap_base_{};
  std::uint64_t heap_size_{};
  std::uint64_t title_id_{};
  const char* profile_name_{};
  std::string report_path_;
  std::uint64_t scan_offset_{};
  bool report_initialized_{};
  QuestScanView view_{};
  std::vector<QuestDataCandidate> candidates_;
  std::vector<std::uint8_t> buffer_;
};

}  // namespace mhgu::platform::switch_adapter
