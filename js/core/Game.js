/**************************************

GAME CLASS SINGLETON:
Handles the DOM, load, init, update & render loops

**************************************/

(function(exports){

// Singleton
var Game = {};
exports.Game = Game;

// PROPERTIES
// Game.width/height are logical world dimensions. The original composition
// remains 960x540, while the logical viewport expands to fit the display.
Game.baseWidth = 960;
Game.baseHeight = 540;
Game.width = Game.baseWidth;
Game.height = Game.baseHeight;
Game.stats = true;
Game.viewport = new Viewport(Game.baseWidth, Game.baseHeight);
Game.viewportMetrics = null;
Game.viewportScale = 1;
Game._baseLayouts = [];
Game._viewportFills = [];
Game._baseAnchors = [];
Game._viewportResizeScheduled = false;
Game._viewportListeners = null;
Game._fullscreenKeyHandler = null;

// Fixed 960x540 artwork (title cards, quote cards, credits) is kept at its
// original size and translated into the center of the expanded world.
Game.getBaseOffset = function(){
	return {
		x: Game.viewportMetrics ? Game.viewportMetrics.extraLeft : 0,
		y: Game.viewportMetrics ? Game.viewportMetrics.extraTop : 0
	};
};
Game.baseToWorldPosition = function(x, y){
	var offset = Game.getBaseOffset();
	return { x: x + offset.x, y: y + offset.y };
};
Game.toBasePosition = function(screenPosition){
	var world = Game.toWorldPosition(screenPosition);
	var offset = Game.getBaseOffset();
	return { x: world.x - offset.x, y: world.y - offset.y };
};

// Register non-World coordinates (for example, AI avoid zones) that belong
// to the original 960x540 composition. The registry applies only the change
// in base offset, preserving any motion since the object was created.
Game.anchorBasePosition = function(target, x, y){
	var offset = Game.getBaseOffset();
	target.x = x + offset.x;
	target.y = y + offset.y;
	var anchor = null;
	for(var i=0;i<Game._baseAnchors.length;i++){
		if(Game._baseAnchors[i].target === target){
			anchor = Game._baseAnchors[i];
			break;
		}
	}
	if(!anchor){
		anchor = { target: target, offsetX: offset.x, offsetY: offset.y };
		Game._baseAnchors.push(anchor);
	}else{
		anchor.offsetX = offset.x;
		anchor.offsetY = offset.y;
	}
	return target;
};
Game.unanchorBasePosition = function(target){
	Game._baseAnchors = Game._baseAnchors.filter(function(anchor){
		return anchor.target !== target;
	});
};
Game.updateBaseAnchors = function(){
	var offset = Game.getBaseOffset();
	Game._baseAnchors.forEach(function(anchor){
		var dx = offset.x - anchor.offsetX;
		var dy = offset.y - anchor.offsetY;
		anchor.target.x += dx;
		anchor.target.y += dy;
		anchor.offsetX = offset.x;
		anchor.offsetY = offset.y;
	});
};

Game.createBaseLayout = function(){
	var layout = new PIXI.Container();
	layout._WBWWB_BASE_LAYOUT = true;
	Game._baseLayouts.push(layout);
	var offset = Game.getBaseOffset();
	layout.x = offset.x;
	layout.y = offset.y;
	return layout;
};

Game.redrawViewportFill = function(fill){
	fill.clear();
	fill.beginFill(fill._WBWWB_FILL_COLOR, 1);
	fill.drawRect(0, 0, Game.width, Game.height);
	fill.endFill();
};
Game.createViewportFill = function(color){
	var fill = new PIXI.Graphics();
	fill._WBWWB_FILL_COLOR = color;
	Game._viewportFills.push(fill);
	Game.redrawViewportFill(fill);
	return fill;
};

// A conservative cap avoids excessively large WebGL backing buffers on
// high-DPI displays while still supporting common 1x/1.25x/1.5x/2x screens.
Game.getRendererResolution = function(){
	var dpr = (typeof window !== "undefined" && window.devicePixelRatio) ? window.devicePixelRatio : 1;
	return Math.max(1, Math.min(dpr, 2));
};

// Recalculate the logical world and keep the Pixi renderer matched to the
// browser viewport. Pointer coordinates from Pixi are renderer-local CSS
// pixels, so Game.toWorldPosition converts them into logical world units.
Game.resizeViewport = function(screenWidth, screenHeight){
	if(typeof window === "undefined") return null;

	screenWidth = (typeof screenWidth === "number") ? screenWidth : window.innerWidth;
	screenHeight = (typeof screenHeight === "number") ? screenHeight : window.innerHeight;
	if(!screenWidth || !screenHeight){
		screenWidth = Game.baseWidth;
		screenHeight = Game.baseHeight;
	}

	var previousMetrics = Game.viewportMetrics;
	var metrics = Game.viewport.resize(screenWidth, screenHeight);
	Game.viewportMetrics = metrics;
	Game.width = metrics.width;
	Game.height = metrics.height;
	Game.viewportScale = metrics.scale;

	if(Game.renderer){
		var resolution = Game.getRendererResolution();
		if(Game.renderer.resolution !== resolution){
			Game.renderer.resolution = resolution;
			// PixiJS 4.0.3 stores the resolution separately in its interaction
			// manager; keep pointer mapping in sync when moving between displays.
			if(Game.renderer.plugins && Game.renderer.plugins.interaction){
				Game.renderer.plugins.interaction.resolution = resolution;
			}
		}
		Game.renderer.resize(metrics.screenWidth, metrics.screenHeight);
	}

	if(Game.stage){
		Game.stage.scale.x = metrics.scale;
		Game.stage.scale.y = metrics.scale;
		if(PIXI.Rectangle){
			Game.stage.hitArea = new PIXI.Rectangle(0, 0, metrics.width, metrics.height);
		}
	}

	// Keep fixed-size original artwork centered as the adaptive world grows.
	Game.updateBaseAnchors();
	var baseOffset = Game.getBaseOffset();
	Game._baseLayouts.forEach(function(layout){
		layout.x = baseOffset.x;
		layout.y = baseOffset.y;
	});
	Game._viewportFills.forEach(Game.redrawViewportFill);

	// Update shared world objects before the scene-specific resize callback.
	if(Game.scene){
		if(Game.scene.world && Game.scene.world.onViewportResize){
			Game.scene.world.onViewportResize(previousMetrics, metrics);
		}
		if(Game.scene.camera && Game.scene.camera.onViewportResize){
			Game.scene.camera.onViewportResize(previousMetrics, metrics);
		}
		if(Game.scene.shaker && Game.scene.shaker.onViewportResize){
			Game.scene.shaker.onViewportResize(previousMetrics, metrics);
		}
		if(Game.scene.director && Game.scene.director.onViewportResize){
			Game.scene.director.onViewportResize(previousMetrics, metrics);
		}
		if(Game.scene.zoomer && Game.scene.zoomer.onViewportResize){
			Game.scene.zoomer.onViewportResize(previousMetrics, metrics);
		}
	}

	// Scenes can opt into a later layout pass without coupling the viewport
	// manager to individual scene implementations.
	if(previousMetrics && Game.scene && Game.scene.onViewportResize){
		Game.scene.onViewportResize(previousMetrics, metrics);
	}

	return metrics;
};

Game.toWorldPosition = function(screenPosition){
	var scale = Game.viewportMetrics ? Game.viewportMetrics.scale : 1;
	return {
		x: screenPosition.x / scale,
		y: screenPosition.y / scale
	};
};

// PixiJS 4.0.3's RenderTexture.create() returns a texture without the legacy
// renderer reference used by RenderTexture.prototype.render(). Render into it
// through the active renderer instead. Keep skipUpdateTransform=true to retain
// parent transforms (the camera relies on its scene transform), but remove the
// adaptive display scale from the cached world transforms first. Otherwise a
// 2x display scale at 1920x1080 is baked into a 960x540 texture and the TV/photo
// preview is zoomed and cropped.
Game.renderToTexture = function(displayObject, renderTexture, transform){
	if(!Game.renderer || !Game.renderer.render){
		throw new Error("Cannot render to texture before the PixiJS renderer is initialized.");
	}
	if(!renderTexture){
		throw new Error("Cannot render to an empty PixiJS RenderTexture.");
	}

	// Only normalize transforms for objects that belong to the game stage.
	// Recompute the hierarchy with an identity stage scale, then restore the
	// live stage properties. The cached transforms remain in logical-world
	// coordinates for this offscreen render; the next on-screen renderer pass
	// updates them again with the real viewport scale.
	var isStageDescendant = false;
	var ancestor = displayObject;
	while(ancestor){
		if(ancestor === Game.stage){
			isStageDescendant = true;
			break;
		}
		ancestor = ancestor.parent;
	}

	var stage = Game.stage;
	var stageScaleX = stage && stage.scale ? stage.scale.x : 1;
	var stageScaleY = stage && stage.scale ? stage.scale.y : 1;
	var stageParent = stage ? stage.parent : null;
	var temporaryParent = Game.renderer._tempDisplayObjectParent;
	if(isStageDescendant && stage && stage.scale && stage.updateTransform && temporaryParent &&
		(stageScaleX !== 1 || stageScaleY !== 1)){
		try{
			stage.parent = temporaryParent;
			stage.scale.x = 1;
			stage.scale.y = 1;
			stage.updateTransform();
		}finally{
			stage.scale.x = stageScaleX;
			stage.scale.y = stageScaleY;
			stage.parent = stageParent;
		}
	}

	Game.renderer.render(displayObject, renderTexture, true, transform || null, true);
	return renderTexture;
};

// Coalesce resize notifications. Browsers can emit several resize events
// during a drag-resize, orientation change, or fullscreen transition; one
// renderer/layout pass per animation frame is enough.
Game.scheduleViewportResize = function(){
	if(typeof window === "undefined" || Game._viewportResizeScheduled) return;
	Game._viewportResizeScheduled = true;
	var schedule = window.requestAnimationFrame || function(callback){
		return window.setTimeout(callback, 16);
	};
	schedule(function(){
		Game._viewportResizeScheduled = false;
		Game.resizeViewport();
	});
};

Game.getFullscreenElement = function(){
	if(typeof document === "undefined") return null;
	return document.fullscreenElement || document.webkitFullscreenElement ||
		document.mozFullScreenElement || document.msFullscreenElement || null;
};

// Fullscreen must be requested from a direct user gesture. Press F or
// Alt+Enter while the game is focused; Escape exits through the browser.
Game.toggleFullscreen = function(){
	if(typeof document === "undefined") return false;
	var fullscreenElement = Game.getFullscreenElement();
	var action = fullscreenElement ?
		(document.exitFullscreen || document.webkitExitFullscreen || document.mozCancelFullScreen || document.msExitFullscreen) :
		(document.documentElement.requestFullscreen || document.documentElement.webkitRequestFullscreen || document.documentElement.mozRequestFullScreen || document.documentElement.msRequestFullscreen);
	if(!action) return false;
	try{
		var result = fullscreenElement ? action.call(document) : action.call(document.documentElement);
		if(result && typeof result.catch === "function"){
			result.catch(function(error){
				if(window.console && console.warn) console.warn("WBWWB fullscreen request was rejected:", error);
			});
		}
		return result || true;
	}catch(error){
		if(window.console && console.warn) console.warn("WBWWB fullscreen request failed:", error);
		return false;
	}
};

Game._isEditableTarget = function(target){
	if(!target) return false;
	var tagName = target.tagName ? target.tagName.toLowerCase() : "";
	return tagName === "input" || tagName === "textarea" || tagName === "select" || !!target.isContentEditable;
};

Game.attachDisplayListeners = function(){
	if(typeof window === "undefined") return;
	Game.detachDisplayListeners();

	var resizeHandler = function(){ Game.scheduleViewportResize(); };
	var listenerPairs = [
		[window, "resize", resizeHandler],
		[window, "orientationchange", resizeHandler],
		[document, "fullscreenchange", resizeHandler],
		[document, "webkitfullscreenchange", resizeHandler],
		[window, "pageshow", resizeHandler]
	];
	if(window.visualViewport){
		listenerPairs.push([window.visualViewport, "resize", resizeHandler]);
	}
	listenerPairs.forEach(function(pair){ pair[0].addEventListener(pair[1], pair[2]); });

	Game._fullscreenKeyHandler = function(event){
		if(event.defaultPrevented || event.repeat || event.ctrlKey || event.metaKey || event.shiftKey) return;
		if(Game._isEditableTarget(event.target)) return;
		var key = event.key ? event.key.toLowerCase() : "";
		if(key === "f" || (key === "enter" && event.altKey)){
			event.preventDefault();
			Game.toggleFullscreen();
		}
	};
	window.addEventListener("keydown", Game._fullscreenKeyHandler);
	Game._viewportListeners = listenerPairs;
};

Game.detachDisplayListeners = function(){
	if(Game._viewportListeners){
		Game._viewportListeners.forEach(function(pair){ pair[0].removeEventListener(pair[1], pair[2]); });
		Game._viewportListeners = null;
	}
	if(Game._fullscreenKeyHandler && typeof window !== "undefined"){
		window.removeEventListener("keydown", Game._fullscreenKeyHandler);
		Game._fullscreenKeyHandler = null;
	}
};

// INIT
Game.init = function(HACK){

	// Calculate the initial adaptive viewport before any scenes are created.
	var metrics = Game.resizeViewport(window.innerWidth, window.innerHeight);

	// Set up PIXI at the actual browser viewport size. The root stage is
	// scaled into the logical world coordinate system by resizeViewport().
	Game.renderer = new PIXI.WebGLRenderer(
		metrics.screenWidth,
		metrics.screenHeight,
		{ resolution: Game.getRendererResolution(), autoResize: false }
	);
	Game.renderer.view.style.display = "block";
	document.querySelector("#stage").appendChild(Game.renderer.view);
	Game.stage = new PIXI.Container();
	Game.stage.interactive = true;
	Game.resizeViewport(metrics.screenWidth, metrics.screenHeight);

	// Keep renderer and world dimensions responsive to window, orientation,
	// browser fullscreen, and visual-viewport changes.
	Game.attachDisplayListeners();

	// Mr Doob Stats
	if(Game.stats){
		Game.stats = new Stats();
		Game.stats.showPanel(0); // 0: fps, 1: ms, 2: mb, 3+: custom
		document.body.appendChild(Game.stats.dom);
	}

	// Scene Manager
	Game.scene = null;
	Game.sceneManager = new SceneManager();

	if(HACK){
		// NOT preloader - jump direct to a scene
		Game.loadAssets(function(){ // well, also get preloader assets...
			Game.loadAssets(function(){
				Game.sceneManager.gotoScene(HACK);
				setInterval(Game.update,1000/60);
				Game.animate();
			}, function(){}, false);
		}, function(){}, true);
	}else{
		// Preloader
		Game.loadAssets(function(){
			Game.sceneManager.gotoScene("Preloader");
			setInterval(Game.update,1000/60);
			Game.animate();
		}, function(){}, true);
	}

};

// UPDATE & ANIMATE

Game.paused = false;

Game.update = function(){
	if(Game.paused) return;
	Tween.tick();
	Game.sceneManager.update();
};

Game.animate = function(){
	if(Game.stats) Game.stats.begin();
	if(!Game.paused){
    	Game.renderer.render(Game.stage);
    }
    if(Game.stats) Game.stats.end();
    requestAnimationFrame(Game.animate);
};

// GAME PAUSED?
// ON BLUR & PAUSE

var modal_shade = document.getElementById("modal_shade");
var paused = document.getElementById("paused");
window.onblur = function(){
	if(Game.scene && Game.scene.UNPAUSEABLE) return;
	modal_shade.style.display = "block";
	paused.style.display = "block";
	Game.paused = true;
	Howler.mute(true);
}
modal_shade.onclick = paused.onclick = function(){
	modal_shade.style.display = "none";
	paused.style.display = "none";
	Game.paused = false;
	Howler.mute(false);
};

// LOADING, and ADDING TO MANIFEST.
// TO DO: Progress, too

Game.manifest = {};
Game.manifest2 = {}; // FOR PRELOADER
Game.sounds = {};

Game.loadAssets = function(completeCallback, progressCallback, PRELOADER){

	var manifest = PRELOADER ? Game.manifest2 : Game.manifest;

	// ABSOLUTE NUMBER OF ASSETS!
	var _totalAssetsLoaded = 0;
	var _totalAssetsToLoad = 0;
	for(var key in manifest){
		var src = manifest[key];
		if(src.slice(-5)==".json"){
			// Is Sprite. Actually TWO assets.
			_totalAssetsToLoad += 2;
		}else{
			_totalAssetsToLoad += 1;
		}
	}
	var _onAssetLoad = function(){
		_totalAssetsLoaded++;
		progressCallback(_totalAssetsLoaded/_totalAssetsToLoad); // PROGRESS.
	};

	// META: Groups To Load – just images & sounds
	var _groupsToLoad = PRELOADER ? 1 : 2;
	var _onGroupLoaded = function(){
		_groupsToLoad--;
		if(_groupsToLoad==0) completeCallback(); // DONE.
	};

	// Howler
	var _soundsToLoad = 0;
	var _onSoundLoad = function(){
		_soundsToLoad--;
		_onAssetLoad();
		if(_soundsToLoad==0) _onGroupLoaded();
	};

	// PIXI
	var loader = PIXI.loader;
	var resources = PIXI.loader.resources;

	for(var key in manifest){

		var src = manifest[key];

		// Is MP3. Leave it to Howler.
		if(src.slice(-4)==".mp3"){
			var sound = new Howl({ src:[
				src.slice(0, src.length-4)+".opus",
				src.slice(0, src.length-4)+".m4a",
				src
			] });
			_soundsToLoad++;
			sound.once('load', _onSoundLoad);
			Game.sounds[key] = sound;
			continue;
		}

		// Otherwise, is an image. Leave it to PIXI.
	    loader.add(key, src);

	}

	// PIXI
	loader.on('progress',_onAssetLoad);
	loader.once('complete', _onGroupLoaded);
	loader.load();

};

// Add To Manifest
Game.addToManifest = function(keyValues, PRELOADER){
	var manifest = PRELOADER ? Game.manifest2 : Game.manifest;
	for(var key in keyValues){
		manifest[key] = keyValues[key];
	}
};

})(window);
