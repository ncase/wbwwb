# We Become What We Behold — Modernized

A technical modernization of the open-source `ncase/wbwwb` codebase.

**[Play the original](https://ncase.itch.io/wbwwb)**

## What changed

- PixiJS 4.x runtime replaced with pinned PixiJS 8.19.0.
- Howler upgraded to 2.2.4.
- Legacy Pixi loader/rendering/interaction calls are bridged in `js/lib/pixi-compat.js`.
- Renderer initialization and asset loading are modernized without rewriting scene content.
- English is the canonical locale.
- Automatic language detection supports English, German, Spanish, Turkish, Portuguese and Persian.
- Unsupported languages and missing translation keys safely fall back to English.
- `?lang=tr` / `?locale=tr` and `localStorage["wbwwb.locale"]` can select a locale.
- The former Chinese fork localization is removed from runtime data.
- HTML structure and repository hygiene are fixed.
- A localization consistency audit is included.

## Scene/content preservation

The modernization deliberately leaves `js/scenes/*`, game assets, sprites, sounds and story/content data unchanged. Compatibility code is isolated to the runtime/helper layer wherever possible.

## Run locally

Serve the repository over HTTP(S):

```bash
python3 -m http.server 8000
```

Then open `http://localhost:8000/`.

Do not use `file://`; WebGL and asset loading are browser-origin sensitive.

## Locale examples

```text
/?lang=en
/?lang=tr
/?lang=de
/?lang=es
/?lang=pt
/?lang=fa
```

## Audit

```bash
npm install
npm run audit:locales
```

A real browser/WebGL session is still required for final visual regression testing.

## Licensing and third-party code

The original project is released under CC0. Third-party libraries remain under their own licenses:

- PixiJS — MIT
- Howler.js — MIT
- CreateJS TweenJS 0.6.2 — retained for compatibility
- stats.js — optional developer diagnostic

See `LICENSE` for the repository's CC0 text and `docs/MODERNIZATION.md` for the technical audit scope.
