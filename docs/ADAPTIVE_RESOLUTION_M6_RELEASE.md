# Adaptive Resolution M6 — Final Release Package

## Release summary

M6 packages the tested M5.1 adaptive-resolution implementation as version 6.0.0 and adds release hygiene: a valid HTML document shell, a local favicon, reproducible npm commands, portable test runners, and this release/QA record.

The game remains based on Nicky Case's original `ncase/wbwwb` project. The original 960×540 composition remains the visual reference frame. On displays with a different aspect ratio, the logical world expands horizontally and/or vertically while preserving the original composition's size and aspect ratio.

## Features retained from M1–M5.1

- Adaptive viewport geometry around the 960×540 reference composition.
- PixiJS 4.0.3 renderer resize, device-pixel-ratio synchronization, and stage scaling.
- Screen-to-world pointer conversion for mouse/touch interaction.
- Centered fixed-composition scenes and expanded gameplay world bounds.
- Resize-aware character/story placement, camera movement, and Director transitions.
- Fullscreen shortcuts: **F** and **Alt+Enter**; native browser fullscreen shortcuts remain available.
- Startup translation initialization with English as the default while retaining translation dictionaries.
- PixiJS 4.0.3 RenderTexture factory/render API migration.
- TV/offscreen preview correction that removes the adaptive root-stage scale while retaining nested camera/scene transforms.

## Run locally

Serve the project root over HTTP (do not open `index.html` through `file://`):

```sh
python -m http.server 8000
```

Then open `http://localhost:8000/`.

## Automated validation

No npm packages are required. With Node.js 16 or newer:

```sh
npm test
npm run check
npm run validate
```

`npm test` runs every `tests/*.test.js` suite. `npm run check` runs `node --check` on project and test JavaScript while excluding the bundled vendor library directory `js/lib/`.

## Validation record

The M5.1 baseline passed all eight automated suites and 52 non-vendor JavaScript syntax checks before M6 packaging. The M6 release runners are added to make those checks repeatable on Windows, macOS, and Linux.

Project-owner browser checks confirmed:

- The M4.2 startup/black-screen issue is resolved in the tested build.
- The M5.1 TV preview no longer appears zoomed/cropped in the tested build.

The following still require a complete manual desktop-browser pass before claiming exhaustive visual QA:

- Fullscreen shortcut and repeated resize lifecycle on the target Windows browser.
- Mouse/touch and camera photo framing across all target aspect ratios.
- All three acts, transitions, credits, and post-credits sequence across 16:9, 16:10, 21:9, and 32:9.

Automated tests do not substitute for WebGL rendering and full gameplay observation.

## Attribution and licensing

This is an adaptation of Nicky Case's `We Become What We Behold` source project: https://github.com/ncase/wbwwb . Preserve the original README's public-domain/CC0 statement and its individual third-party asset credits. In particular, the original README records separate licenses/attributions for some sounds and libraries; do not treat every bundled third-party asset as automatically covered by the project's CC0 dedication.

This release does not claim ownership of the original game or third-party content.
