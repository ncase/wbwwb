/*
 * WBWWB PixiJS compatibility layer.
 *
 * Runtime code only: this file adapts the game's 2016-era PixiJS calls to
 * the modern PixiJS v8 API. It deliberately does not contain scene/content
 * logic. Keeping this boundary makes future scene rewrites optional.
 */
(function (global) {
  "use strict";

  var PIXI = global.PIXI;
  if (!PIXI) throw new Error("PixiJS must be loaded before pixi-compat.js");

  var legacyResources = Object.create(null);
  var queue = [];
  var listeners = { progress: [], complete: [] };

  function emit(type, value) {
    listeners[type].slice().forEach(function (fn) { fn(value); });
  }

  var loader = {
    resources: legacyResources,

    add: function (name, src) {
      queue.push({ name: name, src: src });
      return loader;
    },

    on: function (event, fn) {
      if (listeners[event]) listeners[event].push(fn);
      return loader;
    },

    once: function (event, fn) {
      if (!listeners[event]) return loader;
      var wrapper = function (value) {
        loader.off(event, wrapper);
        fn(value);
      };
      listeners[event].push(wrapper);
      return loader;
    },

    off: function (event, fn) {
      if (!listeners[event]) return loader;
      listeners[event] = listeners[event].filter(function (item) {
        return item !== fn;
      });
      return loader;
    },

    load: function (callback) {
      var items = queue.splice(0);
      if (!items.length) {
        emit("progress", 1);
        emit("complete", loader, {});
        if (callback) callback(loader, {});
        return Promise.resolve(loader);
      }

      var completed = 0;

      function updateProgress() {
        completed += 1;
        emit("progress", completed / items.length);
      }

      return items.reduce(function (promise, item) {
        return promise.then(function () {
          return PIXI.Assets.load(item.src).then(function (asset) {
            var resource = {
              name: item.name,
              url: item.src,
              data: asset && asset.data ? asset.data : null,
              texture: asset && asset.textures ? null : asset,
              spritesheet: asset && asset.textures ? asset : null
            };

            legacyResources[item.name] = resource;
            updateProgress();
          });
        });
      }, Promise.resolve()).then(function () {
        emit("complete", loader, legacyResources);
        if (callback) callback(loader, legacyResources);
        return loader;
      });
    }
  };

  PIXI.loader = loader;

  PIXI.extras = PIXI.extras || {};
  if (!PIXI.extras.MovieClip) {
    PIXI.extras.MovieClip = PIXI.AnimatedSprite;
  }

  var NativeRenderTexture = PIXI.RenderTexture;

  function LegacyRenderTexture(renderer, width, height) {
    var texture = NativeRenderTexture.create({ width: width, height: height });

    texture.render = function (container, transform) {
      var options = {
        target: texture,
        container: container
      };
      if (transform) options.transform = transform;
      renderer.render(options);
    };

    return texture;
  }

  PIXI.RenderTexture = LegacyRenderTexture;

  var NativeText = PIXI.Text;
  PIXI.Text = class LegacyText extends NativeText {
    constructor(text, style) {
      if (typeof text === "string") {
        super({ text: text, style: style || {} });
      } else {
        super(text);
      }
    }
  };

  var eventProperties = {
    mousemove: "mousemove",
    mousedown: "mousedown",
    mouseup: "mouseup",
    mouseover: "mouseover",
    mouseout: "mouseout",
    touchstart: "touchstart",
    touchmove: "touchmove",
    touchend: "touchend"
  };

  Object.keys(eventProperties).forEach(function (property) {
    var eventName = eventProperties[property];
    Object.defineProperty(PIXI.Container.prototype, property, {
      configurable: true,
      enumerable: false,
      get: function () {
        this.__wbwwbHandlers = this.__wbwwbHandlers || {};
        return this.__wbwwbHandlers[property] || null;
      },
      set: function (handler) {
        this.__wbwwbHandlers = this.__wbwwbHandlers || {};

        var previous = this.__wbwwbHandlers[property];
        if (previous) this.off(eventName, previous);

        if (typeof handler !== "function") {
          this.__wbwwbHandlers[property] = null;
          return;
        }

        var wrapped = function (event) {
          handler({
            data: {
              global: event.global
            },
            originalEvent: event.nativeEvent || event.originalEvent || null
          });
        };

        this.__wbwwbHandlers[property] = wrapped;
        this.eventMode = "static";
        this.on(eventName, wrapped);
      }
    });
  });

  if (!Object.getOwnPropertyDescriptor(PIXI.Container.prototype, "interactive")) {
    Object.defineProperty(PIXI.Container.prototype, "interactive", {
      configurable: true,
      enumerable: false,
      get: function () {
        return this.eventMode === "static" || this.eventMode === "dynamic";
      },
      set: function (enabled) {
        this.eventMode = enabled ? "static" : "passive";
      }
    });
  }

  global.WBWWBEnableStageInteraction = function (stage, width, height) {
    stage.eventMode = "static";
    stage.hitArea = new PIXI.Rectangle(0, 0, width, height);
  };
})(window);
