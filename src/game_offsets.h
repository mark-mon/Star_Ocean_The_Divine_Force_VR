#pragma once
#include <cstdint>

// Addresses inside SO6.exe (Steam build, PE timestamp 1669372560), as RVAs from the module base.
// Source: static analysis in so6vr/re/camera_report.md. Verify at runtime before relying on them.
namespace so6vr::off {

// ---- globals ----
constexpr uintptr_t kSceneRoot          = 0x248F750; // Aska scene/world root pointer
constexpr uintptr_t kSceneRoot_MainCam  = 0x590;     // root + 0x590 -> main Aska camera
constexpr uintptr_t kRenderer           = 0x248FA88; // Renderer*
constexpr uintptr_t kCbPrePrepare       = 0x257DE40; // void(*)(void*) called before Renderer::Prepare (null by default)
constexpr uintptr_t kCbPostPrepare      = 0x257DE48; // void(*)(void*) tail-called after Renderer::Prepare (null by default)
constexpr uintptr_t kViewInfoRing       = 0x253CB38; // *ring + 0x10 + idx*0x530 = ViewInfo snapshot
constexpr uintptr_t kViewInfoWriteIdx   = 0x253D3F8; // int
constexpr uintptr_t kViewInfoReadIdx    = 0x253D400; // int
constexpr uintptr_t kViewInfoStride     = 0x530;

// ---- functions ----
constexpr uintptr_t kFnUpdateCamera     = 0x18130C;  // void UpdateCamera(AskaCamera*)
constexpr uintptr_t kFnRendererPrepare  = 0x165888;
constexpr uintptr_t kVtAskaCamera       = 0x1536B68; // vtable of the Aska camera object
constexpr uintptr_t kFnAdvanceRenderView = 0x29D5E8; // void(): render side moves to the next ViewInfo slot
                                                     // (counter 0x253D410++, read idx = counter & 1)

// ---- Aska camera object (0x8C0 bytes) ----
constexpr uintptr_t kCamWorld           = 0x30;   // float4x4 row-major, translation in [3],[7],[11]
constexpr uintptr_t kCamFlags110        = 0x110;  // bit0 world dirty, bit2 view valid
constexpr uintptr_t kCamView            = 0x120;  // float4x4 = inverse(world)
constexpr uintptr_t kCamProj            = 0x420;  // float4x4 with TAA jitter
constexpr uintptr_t kCamViewProj        = 0x4E0;  // float4x4
constexpr uintptr_t kCamPos             = 0x560;  // float4
constexpr uintptr_t kCamProjNoJitter    = 0x5D0;  // float4x4
constexpr uintptr_t kCamLensShift       = 0x650;  // float2, film-back units
constexpr uintptr_t kCamJitter          = 0x660;  // float2
constexpr uintptr_t kCamAspect6F4       = 0x6F4;  // float
constexpr uintptr_t kCamFocal70C        = 0x70C;  // float (= m00)
constexpr uintptr_t kCamNear            = 0x710;  // float
constexpr uintptr_t kCamFar             = 0x714;  // float
constexpr uintptr_t kCamLens            = 0x720;  // LensParams*; +0x10 = full FOV (radians)
constexpr uintptr_t kCamFlags7C2        = 0x7C2;  // uint8: bit0 ortho, bit1 projection dirty, bit5 view dirty
constexpr uintptr_t kCamDepthNear       = 0x7D8;  // float (1.0 = reversed-Z)
constexpr uintptr_t kCamDepthFar        = 0x7DC;  // float
constexpr uintptr_t kCamFilmAspect      = 0x7E0;  // float
constexpr uintptr_t kCamFilmW           = 0x7E4;  // float
constexpr uintptr_t kCamFilmH           = 0x7E8;  // float
constexpr uintptr_t kLensFov            = 0x10;   // float radians

// ---- ViewInfo snapshot (0x530 bytes) ----
constexpr uintptr_t kViView             = 0x000;
constexpr uintptr_t kViProj             = 0x080;
constexpr uintptr_t kViViewProj         = 0x100;
constexpr uintptr_t kViCamPos           = 0x4D0;
constexpr uintptr_t kViFov              = 0x510;
constexpr uintptr_t kViAspect           = 0x514;
constexpr uintptr_t kViNear             = 0x518;
constexpr uintptr_t kViFar              = 0x51C;

} // namespace so6vr::off
