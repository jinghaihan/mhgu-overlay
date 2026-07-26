#include "mhgu/platform/switch/dmnt_memory.hpp"

#ifdef __SWITCH__
#include "dmntcht.h"
#endif

namespace mhgu::platform::switch_adapter {

bool DmntMemoryAccess::read(
    const std::uint64_t address,
    void* destination,
    const std::size_t size
) {
#ifdef __SWITCH__
    return R_SUCCEEDED(
        dmntchtReadCheatProcessMemory(address, destination, size)
    );
#else
    static_cast<void>(address);
    static_cast<void>(destination);
    static_cast<void>(size);
    return false;
#endif
}

bool DmntMemoryAccess::write(
    const std::uint64_t address,
    const void* source,
    const std::size_t size
) {
#ifdef __SWITCH__
    return R_SUCCEEDED(
        dmntchtWriteCheatProcessMemory(address, source, size)
    );
#else
    static_cast<void>(address);
    static_cast<void>(source);
    static_cast<void>(size);
    return false;
#endif
}

}  // namespace mhgu::platform::switch_adapter
