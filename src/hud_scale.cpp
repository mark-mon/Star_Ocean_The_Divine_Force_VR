#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <cmath>
#include <cstring>
#include "hud_scale.h"
#include "log.h"

namespace so6vr {
namespace {

// Area (box) filter: each output pixel averages its whole footprint in the source, using n x n bilinear
// taps spread evenly across it (n = footprint size in source texels, so every source texel contributes).
// The HUD is premultiplied, so colour and alpha average together correctly.
const char* kShader = R"(
cbuffer C : register(b0) {
    float4 srcTexel;   // xy: 1 / source size
    float4 opts;       // x: decode sRGB, y: taps per axis, zw: footprint in source texels
};
Texture2D tex : register(t0);
SamplerState samp : register(s0);

struct V { float4 pos : SV_Position; float2 uv : TEXCOORD0; };

V vsMain(uint id : SV_VertexID) {
    V o;
    float2 uv = float2((id << 1) & 2, id & 2);
    o.uv = uv;
    o.pos = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
    return o;
}

float3 SrgbToLinear(float3 c) {
    return c <= 0.04045 ? c / 12.92 : pow((c + 0.055) / 1.055, 2.4);
}

float4 psMain(V i) : SV_Target {
    int n = (int)opts.y;
    float2 stepUv = opts.zw * srcTexel.xy / n;
    float2 start = i.uv - stepUv * (n - 1) * 0.5;
    float4 acc = 0;
    [loop] for (int y = 0; y < n; ++y)
        [loop] for (int x = 0; x < n; ++x)
            acc += tex.SampleLevel(samp, start + stepUv * float2(x, y), 0);
    acc /= (float)(n * n);
    if (opts.x > 0.5) acc.rgb = SrgbToLinear(acc.rgb);
    return acc;
}
)";

struct Consts { float srcTexel[4], opts[4]; };

template <typename T> void Rel(T*& p) { if (p) { p->Release(); p = nullptr; } }

} // namespace

bool HudScaler::Init(ID3D11Device* dev) {
    if (ready_) return true;
    dev_ = dev;
    HMODULE comp = LoadLibraryW(L"d3dcompiler_47.dll");
    auto compile = comp ? (pD3DCompile)GetProcAddress(comp, "D3DCompile") : nullptr;
    if (!compile) { LOGE("HUD scaling: d3dcompiler_47.dll not available"); return false; }
    ID3DBlob *vsb = nullptr, *psb = nullptr, *err = nullptr;
    if (FAILED(compile(kShader, strlen(kShader), "hud_scale", nullptr, nullptr, "vsMain", "vs_5_0", 0, 0, &vsb, &err))) {
        LOGE("HUD scaling: VS compile failed: %s", err ? (const char*)err->GetBufferPointer() : "?");
        Rel(err); return false;
    }
    if (FAILED(compile(kShader, strlen(kShader), "hud_scale", nullptr, nullptr, "psMain", "ps_5_0", 0, 0, &psb, &err))) {
        LOGE("HUD scaling: PS compile failed: %s", err ? (const char*)err->GetBufferPointer() : "?");
        Rel(err); Rel(vsb); return false;
    }
    bool ok = SUCCEEDED(dev->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, &vs_)) &&
              SUCCEEDED(dev->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, &ps_));
    Rel(vsb); Rel(psb);
    D3D11_SAMPLER_DESC sd{};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    ok = ok && SUCCEEDED(dev->CreateSamplerState(&sd, &sampler_));
    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth = sizeof(Consts);
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ok = ok && SUCCEEDED(dev->CreateBuffer(&bd, nullptr, &cb_));
    D3D11_RASTERIZER_DESC rd{};
    rd.FillMode = D3D11_FILL_SOLID; rd.CullMode = D3D11_CULL_NONE; rd.DepthClipEnable = TRUE;
    ok = ok && SUCCEEDED(dev->CreateRasterizerState(&rd, &rs_));
    D3D11_BLEND_DESC bl{};
    bl.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    ok = ok && SUCCEEDED(dev->CreateBlendState(&bl, &bs_));
    D3D11_DEPTH_STENCIL_DESC dsd{};
    ok = ok && SUCCEEDED(dev->CreateDepthStencilState(&dsd, &ds_));
    if (!ok) { LOGE("HUD scaling: resource creation failed"); return false; }
    ready_ = true;
    return true;
}

ID3D11ShaderResourceView* HudScaler::GetSrv(ID3D11Texture2D* t) {
    for (auto& e : srvs_) if (e.tex == t) return e.srv;
    for (auto& e : srvs_) {
        if (!e.tex) {
            if (FAILED(dev_->CreateShaderResourceView(t, nullptr, &e.srv))) return nullptr;
            e.tex = t;
            return e.srv;
        }
    }
    ReleaseViews();
    return GetSrv(t);
}

ID3D11RenderTargetView* HudScaler::GetRtv(ID3D11Texture2D* t, DXGI_FORMAT fmt) {
    for (auto& e : rtvs_) if (e.tex == t) return e.rtv;
    for (auto& e : rtvs_) {
        if (!e.tex) {
            D3D11_RENDER_TARGET_VIEW_DESC rd{};
            rd.Format = fmt;
            rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            if (FAILED(dev_->CreateRenderTargetView(t, &rd, &e.rtv))) {
                if (FAILED(dev_->CreateRenderTargetView(t, nullptr, &e.rtv))) return nullptr;
            }
            e.tex = t;
            return e.rtv;
        }
    }
    ReleaseViews();
    return GetRtv(t, fmt);
}

void HudScaler::ReleaseViews() {
    for (auto& e : srvs_) { Rel(e.srv); e.tex = nullptr; }
    for (auto& e : rtvs_) { Rel(e.rtv); e.tex = nullptr; }
}

bool HudScaler::Draw(ID3D11DeviceContext* ctx, ID3D11Texture2D* src, ID3D11Texture2D* dst, DXGI_FORMAT dstFormat,
                     UINT dstW, UINT dstH, bool decodeSrgb) {
    if (!ready_ || !src || !dst || !dstW || !dstH) return false;
    D3D11_TEXTURE2D_DESC sdesc;
    src->GetDesc(&sdesc);
    ID3D11ShaderResourceView* srv = GetSrv(src);
    ID3D11RenderTargetView* rtv = GetRtv(dst, dstFormat);
    if (!srv || !rtv) return false;

    float fx = float(sdesc.Width) / float(dstW), fy = float(sdesc.Height) / float(dstH);
    int n = (int)std::ceil(fx > fy ? fx : fy);
    if (n < 1) n = 1;
    if (n > 8) n = 8;
    Consts c{};
    c.srcTexel[0] = 1.0f / float(sdesc.Width);
    c.srcTexel[1] = 1.0f / float(sdesc.Height);
    c.opts[0] = decodeSrgb ? 1.0f : 0.0f;
    c.opts[1] = float(n);
    c.opts[2] = fx;
    c.opts[3] = fy;
    D3D11_MAPPED_SUBRESOURCE m{};
    if (FAILED(ctx->Map(cb_, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) return false;
    memcpy(m.pData, &c, sizeof(c));
    ctx->Unmap(cb_, 0);

    // The XR thread's context is ours alone, so no state needs saving.
    D3D11_VIEWPORT vp{0, 0, float(dstW), float(dstH), 0, 1};
    ctx->OMSetRenderTargets(1, &rtv, nullptr);
    ctx->OMSetBlendState(bs_, nullptr, 0xffffffff);
    ctx->OMSetDepthStencilState(ds_, 0);
    ctx->RSSetState(rs_);
    ctx->RSSetViewports(1, &vp);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(vs_, nullptr, 0);
    ctx->PSSetShader(ps_, nullptr, 0);
    ctx->PSSetConstantBuffers(0, 1, &cb_);
    ctx->PSSetShaderResources(0, 1, &srv);
    ctx->PSSetSamplers(0, 1, &sampler_);
    ctx->Draw(3, 0);
    ID3D11ShaderResourceView* nullSrv = nullptr;
    ctx->PSSetShaderResources(0, 1, &nullSrv);
    ID3D11RenderTargetView* nullRtv = nullptr;
    ctx->OMSetRenderTargets(1, &nullRtv, nullptr);
    return true;
}

} // namespace so6vr
