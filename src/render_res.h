#pragma once
#include <cstdint>

namespace so6vr {

bool RenderResInstall();                                  // hooks the game's graphics-resolution setting
void RenderResSetHmdRecommended(uint32_t w, uint32_t h);  // XR thread: runtime's recommended per-eye size
void RenderResTick();                                     // game thread, once per frame
void RenderResCurrent(uint32_t& w, uint32_t& h);          // the game's current render size

} // namespace so6vr
