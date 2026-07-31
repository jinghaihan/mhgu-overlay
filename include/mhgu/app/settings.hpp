#pragma once

#include <string>

#include "mhgu/core/types.hpp"

namespace mhgu::app {

class SettingsStore {
public:
  explicit SettingsStore(std::string path);

  core::CoreSettings load() const;
  bool save(const core::CoreSettings& settings) const;

private:
  std::string path_;
};

}  // namespace mhgu::app
