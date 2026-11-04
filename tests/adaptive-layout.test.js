// Run with: node tests/adaptive-layout.test.js
var fs = require("fs");
function read(path){ return fs.readFileSync(path, "utf8"); }
var game = read("js/core/Game.js");
var world = read("js/game/World.js");
var preloader = read("js/scenes/Scene_Preloader.js");
var quote = read("js/scenes/Scene_Quote.js");
var credits = read("js/scenes/Scene_Credits.js");
var postCredits = read("js/scenes/Scene_Post_Credits.js");
var endCard = read("js/scenes/Scene_Post_Post_Credits.js");
var director = read("js/game/Director.js");
var camera = read("js/game/Camera.js");
function assert(condition, message){ if(!condition) throw new Error(message); }
assert(game.indexOf("Game.createBaseLayout") >= 0, "base-layout helper missing");
assert(game.indexOf("Game.toBasePosition") >= 0, "base pointer conversion missing");
assert(game.indexOf("Game._baseLayouts.forEach") >= 0, "base layouts are not re-centered on resize");
assert(world.indexOf("Game.createViewportFill(options.bg ? 0x363636 : 0xd8d8d8)") >= 0, "world background extension missing");
assert(world.indexOf("bg.position.x = -100 + bgOffset.x") >= 0, "world background is not centered");
assert(preloader.indexOf("PIXI.RenderTexture.create(Game.baseWidth, Game.baseHeight)") >= 0, "preloader recursive texture should stay base-sized");
assert(preloader.indexOf("new Cursor(self, {baseLayout:true})") >= 0, "preloader cursor is not converted into base coordinates");
assert(quote.indexOf("Game.baseWidth / 2") >= 0, "quote layout should use base center");
assert(credits.indexOf("Game.baseWidth / 2") >= 0, "credits should use base center");
assert(postCredits.indexOf("Game.baseToWorldPosition") >= 0, "post-credits set dressing should follow the base composition");
assert(endCard.indexOf("new Cursor(self, {baseLayout:true})") >= 0, "end-card cursor is not converted into base coordinates");
assert(camera.indexOf("Math.max(Game.width/self.width, Game.height/self.height)") >= 0, "photo cut should cover taller adaptive viewports");
assert(director.indexOf("cam.moveToViewportCenter") >= 0, "Director should delegate resize-aware photo movement to Camera");
console.log("Adaptive layout regression checks passed.");
