# Adaptive Resolution — M2 Renderer Integration

## Scope

M2 connects the M1 viewport metrics to the bundled PixiJS renderer. It does not yet reposition every legacy scene element or change all world/background assets to fill ultrawide and tall aspect ratios. Those are scene/world adaptation tasks for M3 and later.

## Implementation

- Added the `Viewport.js` script before `Game.js` in `index.html`.
- `Game.init()` calculates the logical viewport from the browser's current CSS-pixel dimensions before creating scenes.
- The PixiJS 4.0.3 renderer is created at the browser viewport size, with `resolution` set from `devicePixelRatio` (capped at 2) and `autoResize: false`.
- `Game.resizeViewport()` calls the PixiJS 4.0.3 `renderer.resize(width, height)` method, updates `Game.width`/`Game.height` as logical world bounds, scales the stage root, and updates the stage hit area.
- A window `resize` listener keeps renderer dimensions and adaptive logical dimensions synchronized.
- `Game.toWorldPosition()` converts Pixi interaction coordinates (screen CSS pixels) into logical world coordinates.
- Camera and custom cursor pointer handlers now use `Game.toWorldPosition()`.
- CSS makes the canvas and pause overlay cover the browser viewport rather than a fixed 960x540 rectangle.

## PixiJS compatibility check

The repository bundles PixiJS **4.0.3**. Its bundled renderer base class exposes `resize(width, height)`, stores a `resolution` property, and sizes its backing canvas using that resolution. Its interaction manager maps client coordinates into renderer-local CSS pixels by dividing by the renderer resolution. The implementation uses those v4 APIs and keeps the interaction manager's resolution synchronized when the display's DPR changes.

## Automated tests

Run from the repository root:

```sh
node tests/viewport.test.js
node tests/adaptive-renderer.test.js
```

The integration test stubs the renderer and verifies:

- 2560x1080 maps to a 1280x540 logical world at scale 2.
- Root stage scale and interaction hit area match the logical viewport.
- Center-screen pointer coordinates convert to the logical world center.
- Renderer dimensions remain in screen CSS pixels.
- Renderer and Pixi interaction resolution update together when DPR changes.
- 1920x1200 maps to a 960x600 logical world at scale 2.

## Known limitations before M3

1. Some original scenes place assets at fixed coordinates (the preloader, quote/credits screens, etc.). On non-16:9 viewports these elements are not all re-anchored yet.
2. Existing background art is not yet extended/tiled for every adaptive viewport. The gameplay world/background bounds need M3 treatment.
3. Resizing during a scene updates the renderer and global world dimensions, but existing scene objects will need `onViewportResize` layout handlers in later phases.
4. Camera capture dimensions intentionally remain based on the original 960x540 reference frame (240x135 camera texture). Camera behavior still needs full in-browser regression testing.
5. A full browser run and visual/gameplay regression recording have not been completed in this environment. A headless Chromium smoke-test attempt could not initialize WebGL because its EGL/ANGLE backend could not access a display; this is an environment limitation, not evidence that the game passed or failed in a normal desktop browser. Automated tests are not a substitute for that validation.

## Manual browser check

From this folder, start a local static server (the game loads assets relative to the page):

```sh
python -m http.server 8000
```

Open `http://localhost:8000` in a desktop browser with WebGL enabled. Check the console, then test 1920x1080, 2560x1080, 1920x1200, browser resizing, and fullscreen (F11). Camera/photo and all scene layouts still require later M3/M4 work and regression checks.

## M2 acceptance status

- [x] PixiJS 4.0.3 renderer resize integration.
- [x] Adaptive root-stage scaling.
- [x] Adaptive stage hit area.
- [x] Screen-to-world pointer conversion for camera and custom cursor.
- [x] Full-viewport canvas and pause overlay CSS.
- [x] Automated viewport and renderer-integration tests.
- [ ] Browser-based WebGL smoke test on an actual game session.
- [ ] Scene-specific re-anchoring and background expansion (M3/M4).
