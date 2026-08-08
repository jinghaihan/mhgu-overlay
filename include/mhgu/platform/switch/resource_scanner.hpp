#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "mhgu/platform/switch/memory.hpp"
#include "mhgu/platform/switch/game_profile.hpp"

namespace mhgu::platform::switch_adapter {

enum class ResourceScanStatus : std::uint8_t {
  Idle,
  Scanning,
  Complete,
  ReportFailed,
  InvalidInput,
};

enum class ResourceScanMode : std::uint8_t {
  Initial,
  Filter,
};

struct ResourceAddressCandidate {
  std::uint64_t address{};
  std::uint64_t heap_offset{};
  std::uint32_t zenny{};
  std::uint32_t wycademy_points{};
};

constexpr std::size_t kResourceScanPreviewCapacity = 6;

struct ResourceScanView {
  ResourceScanStatus status{ResourceScanStatus::Idle};
  ResourceScanMode mode{ResourceScanMode::Initial};
  std::uint32_t stage_number{};
  std::uint64_t progress{};
  std::uint64_t progress_total{};
  std::uint32_t expected_zenny{};
  std::uint32_t expected_wycademy_points{};
  std::uint32_t candidate_count{};
  std::uint32_t stored_candidate_count{};
  std::uint32_t skipped_read_count{};
  std::size_t preview_count{};
  std::array<ResourceAddressCandidate, kResourceScanPreviewCapacity> preview{};
};

class ResourceScanner {
public:
  ResourceScanner(
    MemoryAccess& memory,
    const PlayerResourceLayout& layout,
    std::uint64_t heap_base,
    std::uint64_t heap_size,
    std::uint64_t title_id,
    const char* profile_name,
    std::string report_path
  );

  bool start_initial(std::uint32_t zenny, std::uint32_t wycademy_points);
  bool start_filter(std::uint32_t zenny, std::uint32_t wycademy_points);
  void advance(std::size_t byte_budget);
  bool active() const;
  const ResourceScanView& view() const;

private:
  bool begin(
    ResourceScanMode mode,
    std::uint32_t zenny,
    std::uint32_t wycademy_points
  );
  void advance_initial(std::size_t byte_budget);
  void advance_filter();
  void inspect_chunk(
    std::uint64_t address,
    const std::uint8_t* bytes,
    std::size_t body_size,
    std::size_t read_size
  );
  void append_candidate(const ResourceAddressCandidate& candidate);
  void finish();
  bool write_report();

  MemoryAccess& memory_;
  PlayerResourceLayout layout_{};
  std::size_t structure_read_size_{};
  std::uint64_t heap_base_{};
  std::uint64_t heap_size_{};
  std::uint64_t title_id_{};
  const char* profile_name_{};
  std::string report_path_;
  std::uint64_t scan_offset_{};
  std::size_t filter_index_{};
  bool report_initialized_{};
  ResourceScanView view_{};
  std::vector<ResourceAddressCandidate> candidates_;
  std::vector<ResourceAddressCandidate> filtered_candidates_;
  std::vector<std::uint8_t> buffer_;
};

}  // namespace mhgu::platform::switch_adapter
