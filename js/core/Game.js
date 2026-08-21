/**************************************
GAME CLASS SINGLETON:
Handles the DOM, loading, init, update & render loops.

This file is runtime infrastructure. Scene/gameplay content remains unchanged.
**************************************/

(function (exports) {
  "use strict";

  var Game = {};
  exports.Game = Game;

  Game.width = 960;
  Game.height = 540;
  Game.stats = true;
  Game.paused = false;
  Game.assetError = null;

  Game.init = async function (HACK) {
    // PixiJS v8 requires asynchronous renderer initialization.
    Game.renderer = new PIXI.WebGLRenderer();
    await Game.renderer.init({
      width: Game.width,
      height: Game.height,
      preference: "webgl",
      antialias: false,
      resolution: 1
    });

    var stageElement = document.querySelector("#stage");
    if (!stageElement) throw new Error("Missing #stage element");

    stageElement.appendChild(Game.renderer.canvas);

    Game.stage = new PIXI.Container();
    Game.stage.interactive = true;
    WBWWBEnableStageInteraction(Game.stage, Game.width, Game.height);

    // Developer-only FPS diagnostics. Disabled by default in index.html.
    if (Game.stats) {
      Game.stats = new Stats();
      Game.stats.showPanel(0);
      document.body.appendChild(Game.stats.dom);
    }

    Game.scene = null;
    Game.sceneManager = new SceneManager();

    var startScene = HACK || "Preloader";

    await new Promise(function (resolve, reject) {
      Game.loadAssets(function () {
        Game.sceneManager.gotoScene(startScene);
        resolve();
      }, function () {}, true, reject);
    });

    // Keep the original fixed-step gameplay timing while rendering through
    // the modern Pixi renderer.
    Game._updateTimer = window.setInterval(Game.update, 1000 / 60);
    Game.animate();
  };

  Game.update = function () {
    if (Game.paused) return;

    if (typeof Tween !== "undefined" && Tween.tick) {
      Tween.tick();
    }

    Game.sceneManager.update();
  };

  Game.animate = function () {
    if (Game.stats) Game.stats.begin();

    if (!Game.paused && Game.renderer && Game.stage) {
      Game.renderer.render(Game.stage);
    }

    if (Game.stats) Game.stats.end();
    requestAnimationFrame(Game.animate);
  };

  var modal_shade = document.getElementById("modal_shade");
  var paused = document.getElementById("paused");

  window.addEventListener("blur", function () {
    if (Game.scene && Game.scene.UNPAUSEABLE) return;

    modal_shade.style.display = "block";
    paused.style.display = "block";
    Game.paused = true;
    Howler.mute(true);
  });

  modal_shade.onclick = paused.onclick = function () {
    modal_shade.style.display = "none";
    paused.style.display = "none";
    Game.paused = false;
    Howler.mute(false);
  };

  Game.manifest = {};
  Game.manifest2 = {};
  Game.sounds = {};

  Game.showAssetError = function (error) {
    Game.assetError = error;

    var warning = document.getElementById("warning");
    if (!warning) return;

    var message = error && error.message ? error.message : String(error || "Unknown asset error");
    warning.innerHTML =
      "<div>ASSET LOADING ERROR</div>" +
      "<div>A required game asset could not be loaded.</div>" +
      "<div style=\"font-size:12px;word-break:break-word;max-width:90%;margin-top:12px;\">" +
      message.replace(/[&<>\"]/g, function (character) {
        return { "&": "&amp;", "<": "&lt;", ">": "&gt;", "\"": "&quot;" }[character];
      }) +
      "</div>" +
      "<div style=\"margin-top:12px;\">Please refresh the page and try again.</div>";
    warning.style.display = "block";
  };

  Game.loadAssets = function (completeCallback, progressCallback, PRELOADER, errorCallback) {
    var manifest = PRELOADER ? Game.manifest2 : Game.manifest;
    progressCallback = progressCallback || function () {};
    errorCallback = errorCallback || Game.showAssetError;

    var entries = Object.keys(manifest);
    if (!entries.length) {
      progressCallback(1);
      completeCallback();
      return;
    }

    var total = entries.length;
    var completed = 0;

    function progress() {
      completed += 1;
      progressCallback(completed / total);
    }

    var soundPromises = [];
    var imageEntries = [];

    entries.forEach(function (key) {
      var src = manifest[key];

      if (/\.mp3(?:\?|#|$)/i.test(src)) {
        var base = src.replace(/\.mp3(?:\?.*)?$/i, "");
        var sound = new Howl({
          src: [base + ".opus", base + ".m4a", src],
          preload: true
        });

        Game.sounds[key] = sound;

        soundPromises.push(new Promise(function (resolve, reject) {
          var settled = false;

          sound.once("load", function () {
            if (!settled) {
              settled = true;
              progress();
              resolve();
            }
          });

          sound.once("loaderror", function (_id, error) {
            if (!settled) {
              settled = true;
              console.error("Audio failed to load:", src, error);
              progress();
              reject(new Error("Audio failed to load: " + src));
            }
          });
        }));
      } else {
        imageEntries.push({ key: key, src: src });
      }
    });

    var loader = PIXI.loader;
    imageEntries.forEach(function (item) {
      loader.add(item.key, item.src);
    });

    var imagePromise = loader.load().then(function () {
      imageEntries.forEach(function () {
        progress();
      });
    });

    Promise.all([imagePromise].concat(soundPromises))
      .then(function () {
        completeCallback();
      })
      .catch(function (error) {
        console.error("Asset loading failed:", error);

        // IMPORTANT: a required asset failure is fatal for this scene load.
        // Never call completeCallback(), otherwise Scene_Preloader continues
        // with an incomplete resource table and fails later with misleading
        // errors such as "Missing image resource: blackout".
        errorCallback(error);
      });
  };

  Game.addToManifest = function (keyValues, PRELOADER) {
    var manifest = PRELOADER ? Game.manifest2 : Game.manifest;

    for (var key in keyValues) {
      if (Object.prototype.hasOwnProperty.call(keyValues, key)) {
        manifest[key] = keyValues[key];
      }
    }
  };
})(window);
