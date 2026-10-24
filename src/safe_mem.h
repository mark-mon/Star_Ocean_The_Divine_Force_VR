#pragma once
#include <cstddef>
#include <cstdint>

namespace so6vr {

// memcpy guarded by SEH; returns false instead of crashing on a bad pointer.
bool SafeRead(void* dst, const void* src, size_t n);
bool SafeWrite(void* dst, const void* src, size_t n);

template <typename T> bool SafeLoad(uintptr_t addr, T& out) { return SafeRead(&out, reinterpret_cast<const void*>(addr), sizeof(T)); }

} // namespace so6vr
