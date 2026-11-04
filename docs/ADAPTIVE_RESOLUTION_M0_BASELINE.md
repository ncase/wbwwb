# Adaptive Resolution — M0 Baseline Record

## Source baseline

- Upstream project: https://github.com/ncase/wbwwb
- Input archive: `wbwwb-master.zip` supplied for this task
- Rendering library: bundled PixiJS **4.0.3** (`js/lib/pixi.min.js`)
- Reference logical resolution: **960 × 540 (16:9)**
- Renderer creation: `js/core/Game.js`, `new PIXI.WebGLRenderer(Game.width, Game.height)`
- Fixed CSS stage and pause shade: `css/game.css`

## Initial source findings

`Game.width` and `Game.height` are not merely canvas dimensions. They are used as gameplay/world bounds and scene anchors. Relevant consumers found during source scan:

- `js/game/Camera.js`, `Director.js`, `Peep.js`, `World.js`, `ScreenShake.js`, `ScreenZoomOut.js`, `TV.js`
- `js/misc/Cricket.js`
- `js/peeps/LoverPeep.js`, `NormalPeep.js`, `PanicPeep.js`
- `js/scenes/Act_III.js`, `Scene_Credits.js`, `Scene_Meta.js`, `Scene_Post_Credits.js`, `Scene_Post_Post_Credits.js`, `Scene_Preloader.js`, `Scene_Quote.js`

The bundled PixiJS is an old v4.0.3 build. Later renderer integration must use APIs supported by this exact version, not assume current PixiJS APIs. Pointer coordinates and camera photo capture need separate regression checks.

## Baseline status

- [x] Source archive unpacked and structure inspected.
- [x] Fixed resolution declarations and consumers inventoried.
- [x] PixiJS version identified.
- [ ] Original game launched and baseline screenshots captured.
- [ ] Baseline gameplay recording captured for camera, acts, transitions and ending.

The last two items require running the original game in a browser. They are intentionally not marked complete based only on static source inspection.

## M0 constraints

1. Preserve original assets and story/gameplay behavior.
2. Preserve the original 960×540 composition as the reference frame.
3. Adaptive mode expands the logical world around the center on both axes; it does not stretch the original artwork.
4. Do not modify gameplay bounds until renderer scaling and pointer-coordinate conversion are designed and tested together.
