# M4.2 black-screen fix — PixiJS RenderTexture API

The M4.1 hotfix changed legacy `new PIXI.RenderTexture(renderer, width, height)` construction to `PIXI.RenderTexture.create(width, height)`, but it did not migrate the corresponding render calls.

In the bundled PixiJS 4.0.3 build, `RenderTexture.create()` produces a RenderTexture with `legacyRenderer === null`. The compatibility method `RenderTexture.prototype.render()` still delegates to `this.legacyRenderer.render(...)`. Calling it on a factory-created RenderTexture therefore throws before the preloader can draw its recursive preview, which can leave the page looking black.

The fix adds `Game.renderToTexture(displayObject, renderTexture, transform)`, which calls the PixiJS 4 renderer API:

```javascript
Game.renderer.render(displayObject, renderTexture, true, transform || null, true);
```

All three call sites now use this helper:

- `js/scenes/Scene_Preloader.js`
- `js/scenes/Scene_Meta.js`
- `js/game/Camera.js`

The helper preserves the original transform and skip-update behavior while explicitly clearing the render target before drawing.

## Validation

Run all tests from the repository root:

```sh
node tests/viewport.test.js
node tests/adaptive-renderer.test.js
node tests/adaptive-layout.test.js
node tests/adaptive-game-helpers.test.js
node tests/adaptive-gameplay.test.js
node tests/startup-compatibility.test.js
node tests/render-texture-api.test.js
```

Static JavaScript syntax validation is also required. Real desktop-browser WebGL testing remains the final visual check; this container cannot initialize Chromium's WebGL backend.
