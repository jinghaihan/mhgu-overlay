#pragma once

#include "mhgu/core/types.hpp"

namespace mhgu::core {

class Engine {
public:
  CoreOutput
  update(const GameSnapshot& snapshot, const CoreSettings& settings) const;
};

}  // namespace mhgu::core
