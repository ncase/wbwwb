// Run with: node tests/adaptive-renderer.test.js
// Exercises the viewport/renderer integration without requiring a browser or GPU.
var fs = require('fs');
var vm = require('vm');

var document = {
    getElementById: function(){ return { style: {}, onclick: null }; },
    querySelector: function(){ return { appendChild: function(){} }; },
    body: { appendChild: function(){} }
};
var resizeListeners = {};
var context = {
    innerWidth: 2560,
    innerHeight: 1080,
    devicePixelRatio: 2,
    document: document,
    addEventListener: function(name, callback){ resizeListeners[name] = callback; },
    removeEventListener: function(name){ delete resizeListeners[name]; },
    PIXI: {
        Rectangle: function(x, y, width, height){
            this.x = x; this.y = y; this.width = width; this.height = height;
        }
    }
};
context.window = context;
vm.createContext(context);
vm.runInContext(fs.readFileSync('js/core/Viewport.js', 'utf8'), context);
vm.runInContext(fs.readFileSync('js/core/Game.js', 'utf8'), context);

var Game = context.Game;
var calls = [];
Game.renderer = {
    resolution: 1,
    plugins: { interaction: { resolution: 1 } },
    resize: function(width, height){ calls.push([width, height]); }
};
Game.stage = { scale: { x: 1, y: 1 }, hitArea: null };

var wide = Game.resizeViewport(2560, 1080);
if(wide.width !== 1280 || wide.height !== 540 || wide.scale !== 2){
    throw new Error('21:9 viewport dimensions are incorrect: ' + JSON.stringify(wide));
}
if(Game.stage.scale.x !== 2 || Game.stage.scale.y !== 2){
    throw new Error('root stage scale was not applied');
}
if(Game.stage.hitArea.width !== 1280 || Game.stage.hitArea.height !== 540){
    throw new Error('stage hitArea does not match logical viewport');
}
var worldTopLeft = Game.toWorldPosition({ x: 0, y: 0 });
var worldCenter = Game.toWorldPosition({ x: 1280, y: 540 });
var worldBottomRight = Game.toWorldPosition({ x: 2560, y: 1080 });
if(worldTopLeft.x !== 0 || worldTopLeft.y !== 0){
    throw new Error('top-left screen-to-world pointer conversion failed');
}
if(worldCenter.x !== 640 || worldCenter.y !== 270){
    throw new Error('screen-to-world pointer conversion failed');
}
if(worldBottomRight.x !== 1280 || worldBottomRight.y !== 540){
    throw new Error('bottom-right screen-to-world pointer conversion failed');
}
if(Game.renderer.resolution !== 2 || Game.renderer.plugins.interaction.resolution !== 2){
    throw new Error('renderer and interaction resolution did not synchronize');
}
if(calls.length !== 1 || calls[0][0] !== 2560 || calls[0][1] !== 1080){
    throw new Error('renderer.resize was not called with screen dimensions');
}

context.devicePixelRatio = 1;
var tall = Game.resizeViewport(1920, 1200);
if(tall.width !== 960 || tall.height !== 600 || tall.scale !== 2){
    throw new Error('16:10 viewport dimensions are incorrect: ' + JSON.stringify(tall));
}
if(Game.renderer.resolution !== 1 || Game.renderer.plugins.interaction.resolution !== 1){
    throw new Error('resolution change did not update interaction mapping');
}
var tallPointer = Game.toWorldPosition({ x: 960, y: 600 });
if(tallPointer.x !== 480 || tallPointer.y !== 300){
    throw new Error('16:10 pointer mapping failed after resize');
}
if(calls.length !== 2 || calls[1][0] !== 1920 || calls[1][1] !== 1200){
    throw new Error('renderer.resize was not called after resize');
}

console.log('Adaptive renderer tests passed: initial metrics, 21:9 scaling, 16:10 resize, stage hit area, pointer conversion, DPR sync.');
