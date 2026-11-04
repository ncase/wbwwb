// Run with: node tests/render-texture-api.test.js
// Ensures factory-created PixiJS 4 RenderTextures are rendered via the renderer,
// not via the legacy RenderTexture.prototype.render() shim.
var fs = require('fs');
var vm = require('vm');
var assert = function(condition, message){
    if(!condition) throw new Error(message);
};

var rendererCalls = [];
var transformObservations = [];
var context = {
    document: {
        getElementById: function(){ return { style: {}, onclick: null }; }
    },
    console: console,
    PIXI: {}
};
context.window = context;
vm.createContext(context);
vm.runInContext(fs.readFileSync('js/core/Viewport.js', 'utf8'), context);
vm.runInContext(fs.readFileSync('js/core/Game.js', 'utf8'), context);

var target = { legacyRenderer: null };
var displayObject = { name: 'test-display-object' };
var transform = { tx: -160, ty: 0 };
context.Game.renderer = {
    render: function(){ rendererCalls.push(Array.prototype.slice.call(arguments)); }
};
var returned = context.Game.renderToTexture(displayObject, target, transform);
assert(returned === target, 'renderToTexture should return the target texture');
assert(rendererCalls.length === 1, 'renderer.render should be called exactly once');
assert(rendererCalls[0][0] === displayObject, 'display object must be renderer argument 1');
assert(rendererCalls[0][1] === target, 'RenderTexture must be renderer argument 2');
assert(rendererCalls[0][2] === true, 'render target should be cleared before drawing');
assert(rendererCalls[0][3] === transform, 'transform matrix must be forwarded');
assert(rendererCalls[0][4] === true, 'skipUpdateTransform should remain enabled to preserve nested scene transforms');

// The adaptive stage is scaled for the browser viewport (2x at 1920x1080).
// Offscreen captures must retain scene-parent transforms but must not bake
// that root display scale into 960x540 RenderTextures.
var oldRenderer = context.Game.renderer;
var tempParent = { name: 'pixi-temp-root' };
var nestedScene = { localScale: 1.5, computedWorldScale: null };
var stage = {
    scale: { x: 2, y: 2 },
    parent: null,
    updateTransform: function(){
        transformObservations.push({
            scaleX: this.scale.x,
            scaleY: this.scale.y,
            parent: this.parent
        });
        // Simulate PixiJS's recursive Container.updateTransform() for a
        // nested scene container: its own scale must survive stage reset.
        nestedScene.computedWorldScale = this.scale.x * nestedScene.localScale;
    }
};
var child = { parent: stage };
context.Game.stage = stage;
context.Game.renderer = {
    _tempDisplayObjectParent: tempParent,
    render: function(){
        rendererCalls.push(Array.prototype.slice.call(arguments));
    }
};
rendererCalls.length = 0;
context.Game.renderToTexture(child, target, transform);
assert(transformObservations.length === 1, 'stage transforms should be recomputed before offscreen rendering');
assert(transformObservations[0].scaleX === 1 && transformObservations[0].scaleY === 1,
    'offscreen stage transform must be normalized to logical scale 1');
assert(transformObservations[0].parent === tempParent,
    'stage must use PixiJS identity temporary parent while recomputing transforms');
assert(stage.scale.x === 2 && stage.scale.y === 2,
    'live stage display scale must be restored after offscreen transform preparation');
assert(stage.parent === null, 'live stage parent must be restored after transform preparation');
assert(nestedScene.computedWorldScale === 1.5,
    'nested scene transform must be preserved while the adaptive stage scale is removed');
assert(rendererCalls.length === 1 && rendererCalls[0][4] === true,
    'offscreen rendering must keep cached nested transforms after normalization');
context.Game.renderer = oldRenderer;

var files = [
    'js/scenes/Scene_Preloader.js',
    'js/scenes/Scene_Meta.js',
    'js/game/Camera.js'
];
files.forEach(function(file){
    var source = fs.readFileSync(file, 'utf8');
    assert(source.indexOf('renderTexture.render(') === -1,
        file + ' still calls the legacy RenderTexture.render() method');
    assert(source.indexOf('Game.renderToTexture(') >= 0,
        file + ' must use the supported renderer.render() route');
});

console.log('RenderTexture API tests passed: factory-created textures use renderer.render(), not the null legacyRenderer shim.');
