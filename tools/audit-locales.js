/*
 * Static localization audit.
 *
 * Developer utility only; this file is never loaded by the game runtime.
 * It checks that locale overrides only use canonical English keys.
 */
const fs = require("fs");
const vm = require("vm");

const text = fs.readFileSync("js/textStrings.js", "utf8");
const locale = fs.readFileSync("js/locale.js", "utf8");

const context = {
  window: {},
  navigator: { languages: ["en-US"], language: "en-US" },
  location: { search: "" },
  localStorage: { getItem() { return null; }, setItem() {} },
  URLSearchParams,
  console
};
context.window = context;
vm.createContext(context);
vm.runInContext(text, context, { filename: "js/textStrings.js" });
vm.runInContext(locale, context, { filename: "js/locale.js" });

const english = context.WBWWB_EN;
const englishKeys = Object.keys(english).sort();
if (!englishKeys.length) throw new Error("English locale is empty");

const source = locale.match(/var overrides = \{([\s\S]*?)\n  \};/);
if (!source) throw new Error("Could not locate locale overrides");

const overrides = vm.runInNewContext("({" + source[1] + "\n})");
let errors = 0;
for (const [localeName, values] of Object.entries(overrides)) {
  for (const key of Object.keys(values)) {
    if (!Object.prototype.hasOwnProperty.call(english, key)) {
      console.error(`[${localeName}] unknown key: ${key}`);
      errors++;
    }
  }
}

if (errors) {
  process.exitCode = 1;
  console.error(`Locale audit failed with ${errors} error(s).`);
} else {
  console.log(`Locale audit passed: ${englishKeys.length} canonical keys; ${Object.keys(overrides).length} override locales.`);
  console.log("Missing locale keys intentionally fall back to English.");
}
