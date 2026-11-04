
# DEAR ASPIRING TRANSLATORS, PLEASE CHECK THE MAIN PAGE DESCRIPTION FIRST TO SEE IF YOUR TRANSLATION ALREADY EXISTS: https://ncase.itch.io/wbwwb (also check the Issues to see if your translation is already being worked on - collaborate with them if they're stuck!)

*a game about news cycles, vicious cycles, infinite cycles*

**[PLAY](https://ncase.itch.io/wbwwb)**

---

### Made with open culture, for open culture!

I'm releasing all my code and art to the public domain, under the [Creative Commons Zero](http://creativecommons.org/publicdomain/zero/1.0/) un-license. Which means if you wanna remix this to make your own way-too-meta game, or use it in a presentation or classroom or whatever, you already have my permission!

However, not *all* the code/art is mine. Credit's due where credit's due, so...

**CODE:**    
- [PIXI.js](https://github.com/pixijs/pixi.js), for rendering the graphics (MIT License)    
- [Howler.js](https://github.com/goldfire/howler.js), for playing the sounds (MIT License)

**SOUNDS:**    
- [squeak!](https://www.freesound.org/people/ermfilm/sounds/130011/) (CC BY)    
- [park ambience](https://www.freesound.org/people/Mafon2/sounds/274175/) (CC Zero)    
- [camera shutter](https://www.freesound.org/people/uEffects/sounds/207865/) (CC Zero)    
- [single cricket](https://www.freesound.org/people/cs272/sounds/77034/) (CC-BY)    
- [multiple crickets](https://www.freesound.org/people/alienistcog/sounds/124583/) (CC Zero)    
- [news jingle](https://www.freesound.org/people/Tuben/sounds/272044/) (CC Zero)    
- [scream #1](https://www.freesound.org/people/GreatNate98/sounds/353086/) (CC Zero)    
- [scream #2](https://www.freesound.org/people/mariallinas/sounds/222649/) (CC Zero)    
- [gunshot](https://www.freesound.org/people/mitchelk/sounds/136766/) (CC Zero)    
- [gun cocked](https://www.freesound.org/people/martian/sounds/182229/) (CC Zero)    
- [shotgun](https://www.freesound.org/people/lensflare8642/sounds/145209/) (CC Zero)        
- [bloody impact](https://www.freesound.org/people/Hybrid_V/sounds/319590/) (CC BY)        
- [creepy warp sound](https://www.freesound.org/people/Andromadax24/sounds/184476/) (CC BY)        
- [crowd screaming](https://www.freesound.org/people/MultiMax2121/sounds/156860/) (CC Zero)        

**ART:**    
- For the ending, I modified [this photo of a laptop](https://unsplash.com/photos/XyNi3rUEReE). (CC Zero)


## Adaptive resolution development

The adaptive-resolution work preserves the original 960×540 composition as its reference frame while expanding the logical world in both directions to fit different aspect ratios.

- **M0/M1:** Source inventory and adaptive viewport geometry are documented in `docs/ADAPTIVE_RESOLUTION_M0_BASELINE.md`; implementation is in `js/core/Viewport.js`.
- **M2:** PixiJS 4.0.3 renderer resizing, root-stage scaling, high-DPI backing resolution, screen-to-world pointer conversion, and full-window canvas styling are integrated. Details are in `docs/ADAPTIVE_RESOLUTION_M2.md`.
- **M3:** Fixed 960x540 scenes are centered without stretching, gameplay backgrounds and bounds adapt to the expanded world, and resize hooks update camera/scene elements. Details are in `docs/ADAPTIVE_RESOLUTION_M3.md`.
- **M4:** Story-authored positions, AI avoid zones, world props, camera photo movement, Director zoom transitions, and the laptop zoom overlay are hardened for live viewport changes. Details are in `docs/ADAPTIVE_RESOLUTION_M4.md`.
- **Still required:** Desktop browser WebGL visual and end-to-end gameplay regression testing. Automated tests pass, but actual game rendering has not yet been verified in this environment.

Run the automated checks from the repository root:

```sh
node tests/viewport.test.js
node tests/adaptive-renderer.test.js
node tests/adaptive-layout.test.js
node tests/adaptive-game-helpers.test.js
node tests/adaptive-gameplay.test.js
```


- **M4.1 startup compatibility hotfix:** Selects English as the default active translation dictionary for standalone source downloads and updates PixiJS 4 render-texture creation calls. Details: `docs/ADAPTIVE_RESOLUTION_M4_1_HOTFIX.md`.

- **M4.2 black-screen fix:** Migrates the remaining legacy `RenderTexture.render()` calls to `Game.renderer.render()` for PixiJS 4.0.3 factory-created textures. See `docs/ADAPTIVE_RESOLUTION_M4_2_BLACK_SCREEN_FIX.md`.

- **M5.1 TV preview fix:** Removes the adaptive root-stage scale from offscreen RenderTexture transforms while preserving nested scene/camera transforms. Fixes the cropped/zoomed TV preview observed at 1920x1080. Details: `docs/ADAPTIVE_RESOLUTION_M5_1_TV_PREVIEW_FIX.md`.

Additional render-texture regression test:

```sh
node tests/render-texture-api.test.js
```


## Adaptive Resolution 6.0.0

This release packages the adaptive-resolution work around the original 960x540 composition. It expands the visible logical world to fit wider or taller displays instead of stretching the original artwork.

### Quick start

Serve this directory over HTTP (rather than opening `index.html` using `file://`):

```sh
python -m http.server 8000
```

Open `http://localhost:8000/`. Press **F** or **Alt+Enter** to toggle fullscreen; browser-native fullscreen shortcuts remain supported.

### Validation

Node.js 16 or newer is required for the automated checks; no npm dependencies are needed:

```sh
npm test
npm run check
npm run validate
```

See [`docs/ADAPTIVE_RESOLUTION_M6_RELEASE.md`](docs/ADAPTIVE_RESOLUTION_M6_RELEASE.md) for the release scope, manual QA status, and attribution notes. The earlier milestone implementation notes remain in `docs/`.

An unofficial adaptive-resolution adaptation of [We Become What We Behold](https://github.com/ncase/wbwwb).

## M6.0.0

This project preserves the original 960×540 composition while adapting the visible game world to wider and taller displays.

### Features

- Adaptive horizontal and vertical viewport expansion
- PixiJS 4.0.3 renderer integration
- Fullscreen and resize handling
- Pointer coordinate conversion for adaptive viewports
- Camera and TV preview rendering fixes
- English default language selection while retaining translations
- Automated regression tests and release documentation

### Validation reported for the M6 release archive

- 8/8 automated test suites passed
- 54 JavaScript syntax checks passed
- 0 missing local HTML references

The black-screen startup issue and TV preview crop were tested and confirmed fixed by the project owner in a Windows browser.

## Source archive

The complete M6 source archive is available from the project maintainer's ChatGPT conversation as `wbwwb-adaptive-m6-final.zip`.

SHA-256:

`bd526ac5531450ee3fe313b2bdd8cc9762a112a626292b5a8620d91f7af03730`

## Attribution

Based on the original WBWWB project by Nicky Case: https://github.com/ncase/wbwwb

This is an unofficial community adaptation. Consult the original repository for applicable licensing and attribution requirements.