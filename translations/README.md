# Fan Translations

Drop fan translation JSON files for Native WBWWB C++ Port in this folder. The native game scans `translations/*.json` at startup and adds valid fan translations to `Settings > Language`.

## Quick Start

1. Copy `template.json`.
2. Rename the copy to a safe language ID, for example `fr.json`, `es.json`, `pt-BR.json`, `zh-Hans.json`, or `de-fan.json`.
3. Edit the metadata and strings in UTF-8.
4. Start the native game.
5. Open `Settings > Language` and cycle to your language.

The game also checks a `translations/` folder beside the executable and a `translations/` folder in the current working directory. It creates those folders at runtime when it can.

`template.json` includes the current native UI keys, including the TV-style settings pages and the Monitor/display settings. Refresh older fan files from the template when new settings appear; missing keys safely fall back to English while you translate them.

## Required JSON Fields

```json
{
  "schema_version": 1,
  "language_id": "fr",
  "display_name": "Francais",
  "strings": {
    "menu.play": "Jouer"
  }
}
```

Required fields:

- `schema_version`: currently `1`.
- `language_id`: a safe ID using letters, numbers, and hyphens only.
- `display_name`: the name shown in the settings menu.
- `strings`: an object containing localization keys and translated strings.

Optional fields:

- `english_name`
- `authors`
- `game_text_version`
- `notes`

## Safety Rules

Translation files are data only. They cannot run code, load assets, load shaders, play sounds, or reference paths. Unknown, unsafe, non-string, oversized, malformed, or duplicate entries are ignored. Invalid files are skipped with one warning instead of crashing the game.

If your fan translation uses the same `language_id` as an official language, it will not override the official language. Use a distinct ID such as `de-fan`.

## Missing Keys

Missing translated keys fall back to English. This lets you test partial translations while you work.

## Font And Baked Text Limits

Official native packs currently use Latin-script text. Fan translations can be UTF-8, but the native font path may not render every script correctly yet. Unsupported glyphs should not crash the game, but they may appear as missing boxes or fallback symbols.

Some visible text is baked into image assets, especially parts of the original credit images and other art. Those parts may not be fan-translatable yet unless the native port has replaced them with runtime text.

## Testing

After saving your JSON file, restart the game, open `Settings > Language`, and select your language. If the language does not appear, check the console output for one warning explaining why the file was ignored.
