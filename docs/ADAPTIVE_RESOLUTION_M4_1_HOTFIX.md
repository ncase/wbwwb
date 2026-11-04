# M4.1 startup compatibility hotfix

This hotfix addresses two startup/runtime compatibility problems reported while testing M4:

- `js/textStrings.js` contained translation dictionaries but did not assign the `textStrings` dictionary consumed at script-evaluation time by `Act_II.js` and the preloader. The standalone source now defaults to `textStrings_EN`; all existing translation dictionaries are retained.
- Legacy `new PIXI.RenderTexture(renderer, width, height)` calls have been changed to the PixiJS 4 API `PIXI.RenderTexture.create(width, height)` in the camera, preloader, and meta-scene render-texture pools.

The browser's automatic `/favicon.ico` request is unrelated to gameplay. `onboarding.js` is not part of this repository archive; an error from it is likely injected by the local hosting/preview environment and should be evaluated separately if it persists.

Run the full automated suite from this directory:

```sh
node tests/viewport.test.js
node tests/adaptive-renderer.test.js
node tests/adaptive-layout.test.js
node tests/adaptive-game-helpers.test.js
node tests/adaptive-gameplay.test.js
node tests/startup-compatibility.test.js
```

Real browser/WebGL and full-game visual testing are still required.
