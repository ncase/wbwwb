# Adaptive Resolution — M4 Gameplay Regression Hardening

## Goal

Keep existing gameplay objects, story-specific placements, AI avoid zones, camera photography, and Director zoom transitions aligned with the original 960x540 composition when the browser viewport changes.

M4 is source-integrated and automated-test passed. It is not a substitute for a manual desktop WebGL playthrough.

## Changes

### Existing gameplay objects follow the centered composition

`World.onViewportResize` now computes the change in the centered base offset and translates existing world props by that delta. This preserves their placement relative to the original composition when the viewport grows or shrinks. The world transform origin follows the new viewport center; an active zoom pivot moves by the same delta so the visible subject remains framed.

The TV is subsequently re-centered using its existing resize callback. Graphics-only post-credits decorations and background candle props remain handled by the post-credits scene's own resize callback; cricket world props are not translated twice.

### Story-authored positions

Absolute positions that are authored in the 960x540 composition are mapped through `Game.baseToWorldPosition` when created:

- Happy Weirdo, including its pre-murder staging position
- Crazy Peep's scripted entry position
- Evil Hat Peep's scripted entry position and trigger threshold
- Lover Peep's initial position
- Panic Peep lover positions
- Protest animation
- Helping animation and its replacement lovers

Non-World AI avoid zones use `Game.anchorBasePosition`, which tracks base-offset deltas while preserving their current position. Removed avoid zones can be unregistered with `Game.unanchorBasePosition`.

Wandering character bounds now use the expanded logical world dimensions while retaining their original edge margins. This includes Happy Weirdo, Crazy, Angry, Nervous, Hat, Snobby, and Murder peeps. LoverPeep's right-side exit threshold follows the expanded world edge.

### Camera photo movement

The camera's photo render targets remain 240x135 logical pixels, preserving the original image framing. The photo-to-fullscreen movement is now owned by the camera and resolves the viewport center and cover scale dynamically. If the window changes during the move, the transition rebases from the current camera transform and continues without a position or scale jump.

Frozen and streaming camera positions also translate with the centered base offset when the viewport changes.

### Director zoom transitions

Director viewport zooms now accept dynamic target resolvers. During resize, an active transition rebases from the current world pivot and scale, retains its remaining tween progress, and recalculates its destination using the new logical viewport. This covers the TV zoom-out and return-to-world transitions.

### Laptop zoom overlay

The final laptop zoom effect now includes the centered base offset when positioning the laptop overlay, so it stays aligned with the original composition on expanded viewports.

## Automated validation

Run from the repository root:

```sh
node tests/viewport.test.js
node tests/adaptive-renderer.test.js
node tests/adaptive-layout.test.js
node tests/adaptive-game-helpers.test.js
node tests/adaptive-gameplay.test.js
```

`adaptive-gameplay.test.js` verifies:

- Base-anchored avoid zones translate by viewport-offset deltas without losing current motion.
- Existing world props and an active world zoom pivot translate consistently.
- Director viewport transitions rebase continuously and resolve their target against the resized viewport.
- Camera photo-to-center movement rebases continuously and recalculates its cover scale.
- Scripted character, animation and laptop-overlay positions use adaptive coordinate handling.

All non-vendor JavaScript files under `js/` and `tests/` should also pass `node --check`.

## Manual browser verification still required

An attempt to launch Chromium headlessly with SwiftShader could not initialize ANGLE/EGL in this environment (`xcb_connect` / `EGL_NOT_INITIALIZED`). No actual WebGL rendering or gameplay is claimed as verified.

On Windows, run a local static server from the project root:

```powershell
py -m http.server 8000
```

Open `http://localhost:8000` in a desktop browser with WebGL enabled and check:

1. 1920x1080, 2560x1080, 1920x1200 and 3840x1080.
2. Opening preloader and quote composition alignment.
3. Character entrances, walking, wrapping and interactions through all three acts.
4. Camera photo selection, 240x135 crop, fullscreen photo transition, TV cut, zoom-out and reset.
5. Resize during the camera move, TV close-up, viewer zoom-out and return transition.
6. Act III avoid zones, panic lovers, laptop zoom-out and credits/ending sequence.
7. Browser resize and F11 fullscreen transitions, including mouse input at center and all four corners.

## M4 acceptance status

- [x] Existing world props translate with the original composition on resize.
- [x] Zoomed world pivot translates with the same delta.
- [x] Story-authored special character/animation coordinates map to the base composition.
- [x] Avoid zones track base-offset changes and can be unregistered.
- [x] Wandering bounds expand with the logical world.
- [x] Camera photo size remains 240x135.
- [x] Camera photo-to-center tween adapts to resize.
- [x] Director zoom transitions adapt to resize.
- [x] Post-credits props avoid double translation.
- [x] Laptop zoom overlay tracks the centered composition.
- [x] Automated tests and syntax checks pass.
- [ ] Full desktop browser WebGL and end-to-end gameplay verification.
