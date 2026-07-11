# Native WBWWB C++ Port

*A native C++ port of We Become What We Behold, a game about news cycles, vicious cycles, and infinite cycles.*

**[PLAY](https://ncase.itch.io/wbwwb)**

> **Translators:** Check the [main game page](https://ncase.itch.io/wbwwb) and open issues before starting a translation. If one is already in progress, please collaborate with its author.

---

## Made with open culture, for open culture!

I'm releasing all my code and art to the public domain, under the [Creative Commons Zero](http://creativecommons.org/publicdomain/zero/1.0/) un-license. Which means if you wanna remix this to make your own way-too-meta game, or use it in a presentation or classroom or whatever, you already have my permission!

However, not *all* the code/art is mine. Credit's due where credit's due, so...

**CODE:**    
- [PIXI.js](https://github.com/pixijs/pixi.js), for rendering the graphics (MIT License)    
- [Howler.js](https://github.com/goldfire/howler.js), for playing the sounds (MIT License)

**SOUNDS:**    
- [squeak!](https://www.freesound.org/people/ermfilm/sounds/130011/) (CC BY)    
- [park ambience](https://www.freesound.org/people/Mafon2/sounds/274175/) (CC Zero)    
- [camera shutter](https://www.freesound.org/people/uEffects/sounds/207865/) (CC Zero)    
- [single cricket](https://www.freesound.org/people/cs272/sounds/77034/) (CC-BY)    
- [multiple crickets](https://www.freesound.org/people/alienistcog/sounds/124583/) (CC Zero)    
- [news jingle](https://www.freesound.org/people/Tuben/sounds/272044/) (CC Zero)    
- [scream #1](https://www.freesound.org/people/GreatNate98/sounds/353086/) (CC Zero)    
- [scream #2](https://www.freesound.org/people/mariallinas/sounds/222649/) (CC Zero)    
- [gunshot](https://www.freesound.org/people/mitchelk/sounds/136766/) (CC Zero)    
- [gun cocked](https://www.freesound.org/people/martian/sounds/182229/) (CC Zero)    
- [shotgun](https://www.freesound.org/people/lensflare8642/sounds/145209/) (CC Zero)        
- [bloody impact](https://www.freesound.org/people/Hybrid_V/sounds/319590/) (CC BY)        
- [creepy warp sound](https://www.freesound.org/people/Andromadax24/sounds/184476/) (CC BY)        
- [crowd screaming](https://www.freesound.org/people/MultiMax2121/sounds/156860/) (CC Zero)        

**ART:**    
- For the ending, I modified [this photo of a laptop](https://unsplash.com/photos/XyNi3rUEReE). (CC Zero)

---

## Native Build

The `native/` folder contains a C++ port with a default raylib/OpenGL backend and a Windows x64 Vulkan backend. It does not use Electron, Chromium, a webview, or an embedded JavaScript runtime. CMake fetches the pinned dependencies and builds an executable that loads the existing art and audio assets directly from this repo.

```sh
cmake -S native -B native/build
cmake --build native/build --config Release
```

The default backend is `raylib`. You can also select the current renderer explicitly:

```sh
cmake -S native -B native/build -DWBWWB_RENDER_BACKEND=raylib
```

Run the executable from:

```sh
native/build/Release/wbwwb_native.exe
```

On Linux or single-config generators, the binary may be directly under `native/build/`. Debug builds are supported with:

```sh
cmake --build native/build --config Debug
```

For a 32-bit Windows build for older x86 systems, keep it in a separate build folder:

```bat
cmake -S native -B native/build-win32 -G "Visual Studio 17 2022" -A Win32
cmake --build native/build-win32 --config Release
```

The resulting 32-bit executable is:

```sh
native/build-win32/Release/wbwwb_native.exe
```

To create a self-contained 32-bit output folder with the executable and assets:

```bat
cmake --install native/build-win32 --config Release --prefix native/out/wbwwb-win32
```

Run that packaged build from:

```sh
native/out/wbwwb-win32/bin/wbwwb_native.exe
```

There is also a convenience helper:

```bat
native\build-win32.bat
```

The native port includes the preloader, quote scene, main photo/TV gameplay loop, staged escalation, panic/murder sequence, credits, post-credits scene, audio playback, atlas animation loading, letterboxed scaling, native app/window icons, and final-frame visual settings. Player-facing text in the native executable is localized through native language packs.

The main menu has a native TV-style Settings screen. It saves `wbwwb_settings.json` in the current working directory with Audio, Display, Visuals, and Monitor sections. The Monitor page includes display mode, monitor selection, window scale, explicit window size/system resolution, integer scaling, letterbox/overscan, output filter, safe area, frame limit, VSync, and FPS counter options. Visuals include MSAA/FXAA, CRT/static effects, sharpness, gamma, brightness, contrast, black/white levels, TV mask, scanlines, CRT curve, and reduce flashing. Scene drawing, TV captures, gameplay coordinates, and mouse hitboxes remain in the original 960x540 logical space; each backend scales that surface through its final presentation path. Raylib post-processing uses `native/assets/shaders/postprocess.fs`, while Vulkan uses `native/assets/shaders/vulkan/present2d.frag`. Defaults are English, windowed, VSync on, linear output filter, Fit window scale, integer letterbox, 60 FPS, FXAA/MSAA/CRT/noise off, neutral color levels, and full volume. Choosing an explicit Window Size switches Window Scale to Custom so manual resolutions persist. MSAA can be set to Off, 2x, 4x, 8x, or 16x; unsupported sample counts fall back to the highest available level. Native localization data lives under `native/assets/lang/`; exposed complete languages are English, German, Brazilian Portuguese, and Turkish. Fan translators should refresh from `translations/template.json`, which includes the display and HDR setting keys.

The default `WBWWB_RENDER_BACKEND=raylib` build remains the established Windows/Linux and 32-bit path. `WBWWB_RENDER_BACKEND=vulkan` now builds a raylib-free, playable Windows x64 executable. A normal Vulkan launch constructs the same shared `Game`, `SceneManager`, `PreloaderScene`, `SettingsScene`, `QuoteScene`, and `GameplayScene` used by raylib; there is no placeholder gameplay or renderer fallback.

```bat
cmake -S native -B native/build-vulkan -DWBWWB_RENDER_BACKEND=vulkan
cmake --build native/build-vulkan --config Release
native\build-vulkan\Release\wbwwb_native.exe
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

Create a self-contained x64 Vulkan folder with:

```bat
cmake --install native/build-vulkan --config Release --prefix native/out/wbwwb-vulkan-x64
native\out\wbwwb-vulkan-x64\bin\wbwwb_native.exe
```

Shared code receives backend-neutral window, input, capture, audio, renderer, and frame-pipeline services. Vulkan supplies Win32 app/input services, `VulkanRenderer`, `VulkanFramePipeline`, Vulkan render-target capture, and miniaudio playback. It preserves the 960x540 logical game space and supports monitor/display modes, fit/fill and safe-area presentation, VSync, nearest/linear filtering, GPU MSAA, FXAA, CRT/noise/color controls, and an FPS overlay. HDR mode selects a real scRGB or HDR10/PQ swapchain when the display exposes one, submits HDR metadata, and otherwise falls back honestly to SDR. Vulkan image decoding/font rasterization remain Windows-only through WIC/GDI, while Linux and Windows x86 continue to use raylib/OpenGL.

Release and Debug raylib/Vulkan builds and normal launch/close checks pass. All listed Vulkan tests pass in both configurations. `--vulkan-capture-test` verifies 32,768 RGBA bytes and center pixel `(214,32,32,255)`; `--vulkan-boundary-test` drives real shared scenes into `GameplayScene`, takes a non-black selected-resolution camera photo, and exercises the TV path; `--vulkan-resolution-test` verifies 4K scene, post-process, capture, and full-screen picture resources independently of window size and across live resolution changes; HDR validation activates the preferred native scRGB swapchain on the test display, and a 16x MSAA request negotiates to the GPU's supported 8x resolve. Debug validation reports only installed Galaxy overlay layer-name warnings, not repository Vulkan errors. A human end-to-end pass through every late-game branch, credits, translation, and audio cue remains release QA. Visual Studio may also print the existing non-blocking `'pwsh.exe' is not recognized` post-build warning after linking the executable.

## Fan translations

The native port also supports data-only fan translation files without rebuilding. Put UTF-8 JSON files in `translations/` beside the executable, in the current working directory, or in the repository `translations/` folder during development. The game creates those folders when it can, ignores invalid files safely, and shows valid fan packs in `Settings > Language` with a `(Fan)` label. Start from `translations/template.json`; see `translations/README.md` for the schema and rules. Missing fan-translation keys fall back to English, and duplicate IDs such as `en` or `de` do not override official languages.

## Repository validation

Before committing texture or atlas changes, run:

```sh
node scripts/validate-assets.mjs
```

This checks PNG format and size limits, native/browser atlas parity, 2x logical geometry, frame bounds, and unsupported rotated frames.

Published sprites and textures are committed as 2x RGBA8 PNGs. Raw upscaler exports and the generated flat delivery zip are intentionally local-only and excluded by `.gitignore`.
