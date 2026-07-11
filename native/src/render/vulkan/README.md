# Vulkan Backend

This directory contains the raylib-free Windows x64 Vulkan renderer used by the native game. A normal no-flag launch constructs the shared `Game`, `SceneManager`, scenes, and `GameplayScene`; it is no longer a preflight or placeholder menu.

## Build

```bat
cmake -S native -B native/build-vulkan -DWBWWB_RENDER_BACKEND=vulkan
cmake --build native/build-vulkan --config Release
native\build-vulkan\Release\wbwwb_native.exe
```

Package the executable, SPIR-V, translations, icon, and runtime assets with:

```bat
cmake --install native/build-vulkan --config Release --prefix native/out/wbwwb-vulkan-x64
```

The Vulkan build requires a 64-bit Windows toolchain, the Vulkan SDK, and `glslc`. Raylib remains the default backend and the supported Linux/Windows x86 path.

## Runtime

The backend provides:

- Win32 window, monitor, display-mode, DPI, mouse, keyboard, resize, minimize, and clean-close handling through shared app/input interfaces.
- Vulkan instance/device/surface/swapchain lifetime, out-of-date recovery, frame synchronization, and validation support.
- The fixed 960x540 game coordinate system with fit/fill, safe-area, integer-scaling, and letterboxed presentation.
- Backend-neutral texture, font, atlas, arbitrary-quad, shape, scissor, and render-target handles consumed by shared scenes and gameplay.
- Scene, post-process, MSAA, and camera/TV targets that exactly match the selected resolution, with RGBA readback validation and persistent TV photo sampling.
- Universal premultiplied-alpha Catmull-Rom reconstruction for Vulkan sprite, atlas, font, UI, TV, and camera texture draws, with frame-local UV clamping to prevent atlas bleed.
- Optional AMD FidelityFX Super Resolution 1.0.2 photo upscaling, using the reconstructed source plus the official MIT-licensed EASU and RCAS shader implementation at FSR Quality resolution.
- Full-screen menu, quote, and credits pictures decoded and uploaded at the selected resolution, including live reloads when that resolution changes. Sprite atlases retain their authored dimensions so frame coordinates remain exact.
- miniaudio MP3 sound/music playback, volume control, streaming music, looping, and safe no-device fallback.
- VSync and nearest/linear presentation filtering.
- GPU MSAA with 2x/4x/8x/16x requests and fallback to the highest supported sample count.
- FXAA, CRT curve/strength, noise, sharpness, brightness, contrast, gamma, black/white levels, TV border, and scanlines in `present2d.frag`.
- Native HDR swapchains with distinct Off, display-aware Auto, and forced On modes. Auto follows the Windows HDR state and SDR reference-white level for the active monitor; On exposes manual paper-white calibration.
- Calibrated scRGB and HDR10/PQ presentation with BT.2020 conversion, configurable peak brightness and saturated-highlight expansion, matching `VK_EXT_hdr_metadata`, and an 8-bit SDR fallback when HDR is disabled.
- Shared official and fan localization selection. The current GDI atlas covers Basic Latin, Latin-1, and Latin Extended-A; fan packs needing other scripts require broader font-atlas work.

## Validation

```bat
native\build-vulkan\Release\wbwwb_native.exe --smoke-test
native\build-vulkan\Release\wbwwb_native.exe --smoke-test --smoke-game-render
native\build-vulkan\Release\wbwwb_native.exe --vulkan-menu-test
native\build-vulkan\Release\wbwwb_native.exe --vulkan-settings-test
native\build-vulkan\Release\wbwwb_native.exe --vulkan-pause-test
native\build-vulkan\Release\wbwwb_native.exe --vulkan-resolution-test
native\build-vulkan\Release\wbwwb_native.exe --vulkan-renderer-test
native\build-vulkan\Release\wbwwb_native.exe --vulkan-capture-test
native\build-vulkan\Release\wbwwb_native.exe --vulkan-boundary-test
native\build-vulkan\Release\wbwwb_native.exe --vulkan-hdr-test
native\build-vulkan\Release\wbwwb_native.exe --vulkan-hdr-auto-test
native\build-vulkan\Release\wbwwb_native.exe --vulkan-msaa-test
```

`--vulkan-boundary-test` is retained as the migration regression test name. It now drives real shared input through `PreloaderScene`, `QuoteScene`, and `GameplayScene`, takes a real camera photo, verifies that the capture is non-black, and runs 360 shared-game frames.

`--vulkan-capture-test` validates exact offscreen RGBA readback, universal nearest-source texture reconstruction, and preservation of that reconstruction through the FSR 1 EASU/RCAS path. `--vulkan-resolution-test` keeps 3840x2160 scene, post-process, capture, and full-screen picture resources while presenting to a deliberately independent 960x540 swapchain, then verifies that a live 2560x1440 change reallocates all of them before restoring 4K. `--vulkan-hdr-test` forces HDR and verifies manual paper-white/peak calibration; `--vulkan-hdr-auto-test` verifies that Auto follows the active Windows monitor state and adopts its SDR white level. `--vulkan-msaa-test` requests 16x and reports the negotiated device sample count. These flags can be combined to exercise HDR presentation, multisample resolve, and capture together.

## Remaining Scope

- Vulkan surface, WIC image decoding, and GDI font rasterization are Windows-only. Linux continues to use raylib/OpenGL.
- Rotated TexturePacker frames are rejected explicitly. The repository currently contains no rotated atlas frames.
- Complex shaping and scripts outside the current Latin atlas are not implemented.
- Automated tests cover startup, renderer contracts, the first real gameplay/photo cycle, and clean shutdown. A human end-to-end pass through every late-game branch, credits, all translations, and audio cues remains release QA rather than an automated claim.
