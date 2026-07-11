# Native Render Backends

This folder contains the backend-neutral rendering/audio contracts and both native implementations.

- `Renderer.h`, `FramePipeline.h`, and `RenderBackend.*` define the shared game-facing surface.
- `raylib/` implements the default OpenGL path, raylib window/input/audio/capture services, high-resolution final rendering, OpenGL MSAA resolve, and GLSL post-processing. It supports Windows, Linux, and Windows x86 builds.
- `vulkan/` implements the raylib-free Windows x64 path: Win32 window/input/capture services, Vulkan 2D rendering and render targets, shared gameplay, miniaudio playback, final post-processing, GPU MSAA, and calibrated SDR/scRGB/HDR10 presentation with display-aware Auto HDR.
- `miniaudio/` implements the audio backend used by Vulkan.

Shared `Game`, scenes, gameplay, assets, localization, settings, capture, and timing depend on handles/interfaces. Raw raylib resource and window types are confined to `native/src/render/raylib/*`.

Guardrail: run the following before adding renderer work to shared code, and classify every hit as a backend implementation or a neutral compatibility name:

```sh
rg -n "raylib.h|Texture2D|RenderTexture2D|\\bFont\\b|Draw(Texture|Text|Rectangle|Line|Circle)|BeginTextureMode|EndTextureMode" native/src -g "*.h" -g "*.cpp"
```

Current platform limits and validation commands are documented in `native/PORTING.md` and `native/src/render/vulkan/README.md`.
