/**************************************

ADAPTIVE VIEWPORT METRICS

Calculates a logical world viewport that expands around the original
960x540 composition. Renderer integration lives in Game.js; this module
also provides conversion helpers between the expanded world and the
original fixed-size composition.

**************************************/

(function(exports){

function Viewport(baseWidth, baseHeight){
    this.baseWidth = baseWidth || 960;
    this.baseHeight = baseHeight || 540;
    this.metrics = null;
}

// Fit the original composition without cropping or stretching it.
// The logical world grows in either axis when the display has extra room.
Viewport.prototype.calculate = function(screenWidth, screenHeight){
    var width = Number(screenWidth);
    var height = Number(screenHeight);

    if(!isFinite(width) || !isFinite(height) || width <= 0 || height <= 0){
        throw new Error("Viewport dimensions must be positive finite numbers.");
    }

    var scale = Math.min(width / this.baseWidth, height / this.baseHeight);
    var worldWidth = width / scale;
    var worldHeight = height / scale;

    return {
        screenWidth: width,
        screenHeight: height,
        scale: scale,
        width: worldWidth,
        height: worldHeight,
        centerX: worldWidth / 2,
        centerY: worldHeight / 2,
        extraLeft: (worldWidth - this.baseWidth) / 2,
        extraRight: (worldWidth - this.baseWidth) / 2,
        extraTop: (worldHeight - this.baseHeight) / 2,
        extraBottom: (worldHeight - this.baseHeight) / 2,
        aspectRatio: width / height
    };
};

Viewport.prototype.resize = function(screenWidth, screenHeight){
    this.metrics = this.calculate(screenWidth, screenHeight);
    return this.metrics;
};

Viewport.prototype.resizeToWindow = function(){
    if(typeof window === "undefined") return null;
    return this.resize(window.innerWidth, window.innerHeight);
};

Viewport.prototype.getMetrics = function(){
    return this.metrics;
};

// Convert between the expanded world and the original 960x540 composition.
Viewport.prototype.baseToWorldPosition = function(x, y, metrics){
    metrics = metrics || this.metrics;
    if(!metrics) return { x: x, y: y };
    return { x: x + metrics.extraLeft, y: y + metrics.extraTop };
};
Viewport.prototype.worldToBasePosition = function(x, y, metrics){
    metrics = metrics || this.metrics;
    if(!metrics) return { x: x, y: y };
    return { x: x - metrics.extraLeft, y: y - metrics.extraTop };
};
Viewport.prototype.screenToBasePosition = function(x, y, metrics){
    metrics = metrics || this.metrics;
    if(!metrics) return { x: x, y: y };
    return this.worldToBasePosition(x / metrics.scale, y / metrics.scale, metrics);
};

exports.Viewport = Viewport;

})(typeof window !== "undefined" ? window : this);
