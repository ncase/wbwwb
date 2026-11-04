# Adaptive Resolution — M5 Display Lifecycle and QA

## Goal

Harden the adaptive viewport against real desktop display changes while preserving the original WBWWB composition and the M4.2 RenderTexture startup fix.

M5 adds:

- Coalesced viewport updates so a burst of browser resize notifications produces one renderer/layout pass per animation frame.
- Resize notifications for ordinary window resize, orientation changes, browser fullscreen changes, `pageshow`, and `visualViewport.resize` where available.
- Fullscreen shortcuts: **F** or **Alt+Enter** toggles fullscreen; browser **Escape** exits fullscreen.
- Editable-target protection so the fullscreen shortcut does not steal keystrokes from input, textarea, select, or content-editable elements.
- Listener cleanup before reattaching display listeners, preventing duplicate handlers if `Game.init()` is called again.

The original 960x540 logical composition and adaptive expanded-world coordinate model are unchanged. Camera photos remain 240x135 logical pixels.

## Implementation

`js/core/Game.js` now provides:

- `Game.scheduleViewportResize()` — coalesces resize work with `requestAnimationFrame` (16ms timeout fallback).
- `Game.getFullscreenElement()` — detects standard and vendor-prefixed fullscreen state.
- `Game.toggleFullscreen()` — enters/exits fullscreen using the current user gesture and handles rejected requests without an unhandled Promise rejection.
- `Game.attachDisplayListeners()` / `Game.detachDisplayListeners()` — manage resize, orientation, fullscreen, pageshow, visual viewport and keyboard events.
- `Game._isEditableTarget()` — prevents the hotkey from interfering with text entry.

No visible controls were added to the game canvas, keeping the original presentation unchanged. Native browser fullscreen (for example F11) continues to work as well.

## Automated validation

Run from the repository root:

```sh
node tests/viewport.test.js
node tests/adaptive-renderer.test.js
node tests/adaptive-layout.test.js
node tests/adaptive-game-helpers.test.js
node tests/adaptive-gameplay.test.js
node tests/startup-compatibility.test.js
node tests/render-texture-api.test.js
node tests/adaptive-display-lifecycle.test.js
```

The new `adaptive-display-lifecycle.test.js` checks:

- Resize, orientation, fullscreen and visual viewport events are coalesced.
- F and Alt+Enter call the fullscreen API.
- Text-entry targets are ignored.
- Display and keyboard listeners are removed cleanly.

All non-vendor JavaScript source files must also pass `node --check`.

## Windows browser QA checklist

The automated tests do not replace actual WebGL and game-playthrough validation. On Windows, extract the archive into a fresh folder, serve the inner `wbwwb-master` directory, and open `http://localhost:8000/`.

```powershell
py -m http.server 8000
```

Test these display modes:

| Mode | Resolution | Expected adaptive world |
|---|---:|---:|
| 16:9 | 1920x1080 | 960x540 |
| 16:10 | 1920x1200 | 960x600 |
| 21:9 | 2560x1080 | 1280x540 |
| 32:9 | 3840x1080 | 1920x540 |

Checklist:

1. Confirm the preloader, quote, title/ending cards and credits remain centered and undistorted.
2. Resize the browser repeatedly; confirm no black frame, stretched canvas, or duplicate rendering.
3. Toggle fullscreen with **F**, **Alt+Enter**, and the browser's native fullscreen shortcut; exit with **Escape**.
4. Move the camera to the four corners and center; verify pointer/camera alignment and that the camera remains inside the world.
5. Take a photo; verify the original 240x135 framing and photo-to-center transition.
6. Resize while the photo moves to center, during TV close-up, during viewer zoom-out, and while returning to the world.
7. Play all three acts and the post-credits/ending sequence; check character entrances, world edges, avoid zones and credits.
8. Repeat at browser zoom 100% and 125%, plus any high-DPI display available.

## Validation status

- [x] Existing M4.2 startup and RenderTexture fix retained.
- [x] Display lifecycle and fullscreen code added.
- [x] Automated viewport and gameplay suites pass.
- [x] New display lifecycle test passes.
- [x] Non-vendor JavaScript syntax checks pass.
- [ ] Actual Windows WebGL visual test of M5 fullscreen shortcuts and resize lifecycle.
- [ ] Full manual three-act gameplay pass at the four target aspect ratios.

The packaging environment's Chromium navigation is blocked by its administrator policy, including localhost and local-file navigation. Therefore this M5 package is source-integrated and automated-test validated, but M5's new fullscreen/resize behavior still needs the Windows browser QA above. M4.2 itself has already been confirmed working by the project owner.
