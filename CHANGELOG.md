# Changelog

## 6.0.0 — Adaptive Resolution Finalization

- Finalized the adaptive-resolution source package based on the M5.1 build confirmed by the project owner.
- Added a valid HTML document structure with responsive viewport metadata and a local SVG favicon.
- Added `npm test`, `npm run check`, and `npm run validate` scripts; no npm dependencies are required.
- Added a cross-platform Node.js test runner and syntax-check runner.
- Consolidated release notes and manual verification status in `docs/ADAPTIVE_RESOLUTION_M6_RELEASE.md`.
- Preserved the original 960×540 base composition, adaptive expanded world, PixiJS 4.0.3 RenderTexture fixes, and TV preview transform correction.

## 5.1.0 — TV Preview Transform Fix

- Corrected offscreen RenderTexture transform handling so the TV preview is not zoomed/cropped by the adaptive root-stage scale.
- Kept camera and nested scene transforms intact.

## 5.0.0 — Display Lifecycle

- Added resize coalescing, fullscreen/orientation/visualViewport listeners, and F / Alt+Enter fullscreen shortcuts.
