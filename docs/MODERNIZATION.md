# Modernization audit

## Scope

The modernization is based on `enis1enis2/wbwwb` and preserves the scene/content boundary. Scene scripts, story/dialogue, sprites, sounds, spritesheet data and gameplay content are not intentionally rewritten.

## Changes

- PixiJS 4.x runtime replaced with pinned PixiJS 8.19.0.
- Howler upgraded to 2.2.4.
- Legacy `PIXI.loader` calls are bridged to the Promise-based `PIXI.Assets` API.
- Legacy `RenderTexture`, `Text`, `MovieClip` and interaction patterns are bridged in `js/lib/pixi-compat.js`.
- Renderer initialization is asynchronous as required by PixiJS v8.
- The `props = {} || props` helper bug is fixed.
- English is the canonical localization source.
- Supported runtime locales: English, German, Spanish, Turkish, Portuguese and Persian.
- Browser locale detection, query-string selection, localStorage persistence and English fallback are implemented.
- The Chinese fork localization is removed from runtime data.
- Invalid HTML structure is corrected.
- Repository hygiene files and a locale audit utility are included.

## Intentionally retained

TweenJS 0.6.2 and stats.js remain because existing runtime code uses them. The FPS overlay is developer-only and disabled by default.

## Validation

Run:

```bash
npm run audit:locales
```

For final browser validation, serve the repository over HTTP and test preloading, rendering, pointer/touch input, camera capture, audio, scene transitions, pause/resume and every locale. Static checks cannot prove WebGL visual correctness.
