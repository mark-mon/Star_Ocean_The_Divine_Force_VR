#pragma once
#include <d3d11.h>

namespace so6vr {

// XR thread: shrinks the HUD image to the size the headset can actually show (about one HUD texel per
// display pixel), with an area filter. Handing the compositor a texture at twice the display's pixel
// density, without mipmaps, makes thin text alias and shimmer, which reads as blur.
class HudScaler {
public:
    bool Init(ID3D11Device* dev);
    // `src` needs D3D11_BIND_SHADER_RESOURCE; `dst` must allow a render target view in `dstFormat`.
    // `decodeSrgb`: `src` holds sRGB-encoded values in a UNORM format and `dst` is an _SRGB target.
    bool Draw(ID3D11DeviceContext* ctx, ID3D11Texture2D* src, ID3D11Texture2D* dst, DXGI_FORMAT dstFormat,
              UINT dstW, UINT dstH, bool decodeSrgb);
    void ReleaseViews();
    bool Ready() const { return ready_; }

private:
    ID3D11Device* dev_ = nullptr;
    ID3D11VertexShader* vs_ = nullptr;
    ID3D11PixelShader* ps_ = nullptr;
    ID3D11SamplerState* sampler_ = nullptr;
    ID3D11Buffer* cb_ = nullptr;
    ID3D11RasterizerState* rs_ = nullptr;
    ID3D11BlendState* bs_ = nullptr;
    ID3D11DepthStencilState* ds_ = nullptr;
    struct SrvEntry { ID3D11Texture2D* tex; ID3D11ShaderResourceView* srv; };
    struct RtvEntry { ID3D11Texture2D* tex; ID3D11RenderTargetView* rtv; };
    SrvEntry srvs_[8] = {};
    RtvEntry rtvs_[8] = {};
    bool ready_ = false;

    ID3D11ShaderResourceView* GetSrv(ID3D11Texture2D* t);
    ID3D11RenderTargetView* GetRtv(ID3D11Texture2D* t, DXGI_FORMAT fmt);
};

} // namespace so6vr
