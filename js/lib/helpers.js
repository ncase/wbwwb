/*************************************************************
 * RUNTIME HELPERS
 * Utility functions used by the game engine at runtime.
 * - Math.TAU: convenience constant for 2*PI
 * - BEAT: animation timing unit (1 frame)
 * - Tween_get: wrapper around Tween.get with tick-based timing
 * - _s: converts seconds to tick frames
 * - MakeSprite / MakeMovieClip: PIXI asset factory helpers
 *************************************************************/

Math.TAU = Math.PI*2;

// Animation timing helpers
var BEAT = 1;
var Tween_get = function(target, props){
	props = props || {};
	props.useTicks = true;
	return Tween.get(target, props);
}
var _s = function(seconds){
	return Math.ceil(Ticker.framerate*seconds); // converts seconds to ticks
};

// PIXI sprite factory helpers
var MakeSprite = function(textureName){
	return new PIXI.Sprite(PIXI.loader.resources[textureName].texture);
}

// PIXI MovieClip factory helper
var MakeMovieClip = function(resourceName){

	// Build frames from sprite sheet JSON
	var resources = PIXI.loader.resources;
	var resource = resources[resourceName];	
	var numFrames = Object.keys(resource.data.frames).length;
	var frames = [];
	for(var i=0; i<numFrames; i++){
		var str = "0000" + i; // FOUR leading zeroes
		str = str.substr(str.length-4);
		frames.push(PIXI.Texture.fromFrame(resourceName+str));
	}
	var mc = new PIXI.extras.MovieClip(frames);

	// Default state
	mc.gotoAndStop(0);
	mc.anchor.x = 0.5;
	mc.anchor.y = 1.0;

	// Return constructed MovieClip
	return mc;

};