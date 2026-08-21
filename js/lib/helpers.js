/***************
RUNTIME HELPERS
***************/

Math.TAU = Math.PI * 2;

// Animation helpers.
var BEAT = 1;

var Tween_get = function (target, props) {
  // Runtime helper: preserve caller options instead of discarding them.
  props = props || {};
  props.useTicks = true;
  return Tween.get(target, props);
};

var _s = function (seconds) {
  return Math.ceil(Ticker.framerate * seconds);
};

// Image helpers.
var MakeSprite = function (textureName) {
  var resource = PIXI.loader.resources[textureName];
  if (!resource || !resource.texture) {
    throw new Error("Missing image resource: " + textureName);
  }
  return new PIXI.Sprite(resource.texture);
};

// Movie clips are backed by PixiJS v8 AnimatedSprite. The original
// spritesheet naming/order is retained, so this is a technical migration,
// not a content/animation change.
var MakeMovieClip = function (resourceName) {
  var resource = PIXI.loader.resources[resourceName];
  if (!resource || !resource.spritesheet) {
    throw new Error("Missing spritesheet resource: " + resourceName);
  }

  var textures = resource.spritesheet.textures;
  var frameNames = Object.keys(textures).sort(function (a, b) {
    var na = parseInt((a.match(/(\d+)(?:\.[^.]+)?$/) || ["", "0"])[1], 10);
    var nb = parseInt((b.match(/(\d+)(?:\.[^.]+)?$/) || ["", "0"])[1], 10);
    return na - nb || a.localeCompare(b);
  });

  var frames = frameNames.map(function (name) {
    return textures[name];
  });

  var mc = new PIXI.AnimatedSprite(frames);
  mc.gotoAndStop(0);
  mc.anchor.x = 0.5;
  mc.anchor.y = 1.0;

  return mc;
};
