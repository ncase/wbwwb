# Adaptive Resolution M5.1 — TV Preview Transform Fix

## User-observed issue

At 1920x1080, the recursive preloader image inside the illustrated TV was zoomed and cropped: the title/Persian text were cut off horizontally and the red play button was clipped at the bottom.

## Root cause

M2 scales `Game.stage` to map logical world coordinates to the browser viewport. At 1920x1080, that scale is 2. The M4.2 PixiJS 4.0.3 RenderTexture migration correctly switched to `renderer.render(...)`, but intentionally kept `skipUpdateTransform=true` to preserve nested scene transforms used by the camera. That also preserved the adaptive root-stage scale in the cached world transforms. A 960x540 offscreen texture therefore received coordinates already multiplied by 2 and captured a cropped section.

## Fix

`Game.renderToTexture()` now detects whether the requested display object belongs to `Game.stage`. Before rendering, it temporarily parents the stage to PixiJS's identity `_tempDisplayObjectParent`, sets the stage scale to 1, and calls `stage.updateTransform()` to recompute the hierarchy in logical-world coordinates. It then restores the live stage scale and parent and performs the render with `skipUpdateTransform=true`.

Keeping `skipUpdateTransform=true` is deliberate: camera captures still need the nested `scene.graphics` transforms already present in the display object's world transform. The change removes only the browser-fit scale contributed by the root stage.

The next normal on-screen render recomputes the hierarchy using the actual adaptive viewport scale.

## Regression coverage

`tests/render-texture-api.test.js` now verifies that:

- the offscreen hierarchy is recomputed with stage scale 1;
- the PixiJS identity temporary parent is used during that update;
- stage scale and parent are restored afterwards;
- the renderer still receives `skipUpdateTransform=true` so nested scene transforms remain available;
- all RenderTexture call sites use `Game.renderToTexture()` rather than the legacy factory-incompatible `.render()` shim.

## Validation

All 8 automated test suites pass. All 52 non-vendor JavaScript files pass `node --check`. The archive passes ZIP integrity checks. Actual visual confirmation remains a Windows browser task because this build environment cannot initialize WebGL.

## Windows verification

1. Extract this archive into a new folder.
2. Serve the inner `wbwwb-master` directory over HTTP.
3. Open the game at 1920x1080.
4. Confirm that the preloader TV shows the complete title, Persian subtitle, red button, playing-time label and warning without clipping.
5. Test the camera/TV feed during gameplay and resize to an ultrawide aspect ratio.
