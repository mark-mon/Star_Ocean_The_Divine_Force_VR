#pragma once
#include <d3d11.h>

namespace so6vr {

// Render thread, every Present: creates the in-headset settings menu on first use, polls its input and
// hands the menu image (while open) to the XR thread.
void MenuOnPresent(ID3D11Device* dev, ID3D11DeviceContext* ctx);

} // namespace so6vr
