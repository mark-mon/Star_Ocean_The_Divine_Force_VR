#pragma once

namespace so6vr {

// Installs the bootstrap hooks (LoadLibraryExW + DXGI factory creation).
// D3D11 device, swapchain and context hooks are installed lazily as the game creates them.
bool InstallD3DHooks();
void RemoveD3DHooks();

} // namespace so6vr
