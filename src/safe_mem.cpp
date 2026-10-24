// Guarded memory reads: a wrong offset must log a failure, not crash the game.
#include <windows.h>
#include <cstring>
#include "safe_mem.h"

namespace so6vr {

bool SafeRead(void* dst, const void* src, size_t n) {
    if (!src) return false;
    __try {
        memcpy(dst, src, n);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool SafeWrite(void* dst, const void* src, size_t n) {
    if (!dst) return false;
    __try {
        memcpy(dst, src, n);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

} // namespace so6vr
