# Adaptive Resolution — M3 Scene and World Adaptation

## M3 goal

Keep the original 960x540 composition visually centered and undistorted while allowing the playable world to grow horizontally and vertically with the display aspect ratio.

## Implementation

### Shared coordinate/layout helpers

`js/core/Game.js` now provides:

- `Game.getBaseOffset()` — the centered offset of the original 960x540 composition inside the expanded logical world.
- `Game.baseToWorldPosition(x, y)` — maps coordinates authored against the original composition into the adaptive world.
- `Game.toBasePosition(screenPosition)` — maps pointer coordinates into a fixed 960x540 layout.
- `Game.createBaseLayout()` — creates a container that is automatically centered whenever the viewport changes.
- `Game.createViewportFill(color)` — creates a background fill that redraws to the adaptive world dimensions after resize.

`SceneManager` clears the helper registries when replacing a scene so old scenes do not remain referenced by the resize manager.

### Fixed-composition scenes

- Preloader artwork, loading controls, warning text and custom cursor remain in the original 960x540 layout.
- The preloader's recursive render texture remains 960x540 and renders its centered layout without the viewport translation.
- Quote cards and their text remain aligned to the original composition.
- Credits cards and text remain in the original coordinate space.
- The post-credits end card, buttons, social icons and cursor remain centered and use base-layout pointer coordinates.
- Additional viewport area is filled with a compatible background colour rather than stretching the original artwork.

### Adaptive gameplay world

- The original 1160x740 world background is centered around the original composition.
- A matching edge-colour fill extends the background area when the logical viewport is larger than the source texture.
- Character spawning and movement boundaries already use `Game.width` and `Game.height`; they therefore use the expanded world bounds without changing character scale or movement speed.
- The TV and ordinary camera recenter with the expanded viewport.
- The post-credits candles, lovers, crickets and shade overlay are translated with the original composition.
- The recursive promotional scene refreshes its render textures after viewport resizing.
- Screen-shake noise and the laptop zoom overlay cover/track the adaptive viewport.
- The camera photo retains the original 240x135 capture dimensions. Its fullscreen cut uses a cover scale so taller viewports are covered without letterboxing.

## Automated checks

Run from the repository root:

```sh
node tests/viewport.test.js
node tests/adaptive-renderer.test.js
node tests/adaptive-layout.test.js
node tests/adaptive-game-helpers.test.js
```

Also run syntax checks (excluding the minified vendor libraries if desired):

```sh
find js tests -name '*.js' -not -path 'js/lib/*' -print0 | xargs -0 -n1 node --check
```

The automated tests cover viewport geometry, renderer integration, base-layout offsets, adaptive fill dimensions, pointer conversion, and source-level scene integration checks.

## Manual validation still required

A real desktop browser run is still required to validate visuals and PixiJS 4.0.3 WebGL interactions. The Chromium smoke-test attempt in this environment was blocked by the browser execution policy (`ERR_BLOCKED_BY_ADMINISTRATOR`) before the page loaded, so no actual gameplay rendering or screenshots are claimed as verified.

After extracting the archive on Windows:

1. Open a terminal in the project folder.
2. Run `python -m http.server 8000`.
3. Open `http://localhost:8000` in a WebGL-enabled desktop browser.
4. Check the preloader, quote, opening gameplay, camera capture, all acts, credits, post-credits scene and end card.
5. Resize between 1920x1080, 2560x1080, 1920x1200 and 3840x1080; also test fullscreen and windowed transitions.
6. Check mouse/touch input at the edges and center of each viewport.

## M3 acceptance status

- [x] Fixed 960x540 content stays in a centered base layout.
- [x] Preloader, quote, credits and end card use the base layout.
- [x] Base-layout custom cursor coordinates subtract the adaptive offset.
- [x] Gameplay background and dark post-credits scene use adaptive edge fills.
- [x] Post-credits static set dressing follows the centered base composition.
- [x] Gameplay world bounds expand through the existing dynamic `Game.width`/`Game.height` values.
- [x] Camera dimensions remain tied to the original reference frame.
- [x] Camera fullscreen transition covers taller viewports.
- [x] Resize hooks update world, TV, camera, overlays and recursive scene render targets.
- [x] Automated tests and non-vendor JavaScript syntax checks pass.
- [ ] Desktop browser WebGL visual and gameplay regression test.

M3 is source-integrated and automated-test passed, but is not yet browser-verified.
