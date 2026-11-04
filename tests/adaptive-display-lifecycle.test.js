// Run with: node tests/adaptive-display-lifecycle.test.js
// Verifies coalesced viewport lifecycle events and gesture-driven fullscreen.
var fs = require('fs');
var vm = require('vm');

function assert(condition, message){
    if(!condition) throw new Error(message);
}

function createEventTarget(){
    return {
        listeners: {},
        addEventListener: function(name, callback){
            (this.listeners[name] || (this.listeners[name] = [])).push(callback);
        },
        removeEventListener: function(name, callback){
            var list = this.listeners[name] || [];
            this.listeners[name] = list.filter(function(item){ return item !== callback; });
        },
        dispatch: function(name, event){
            (this.listeners[name] || []).slice().forEach(function(callback){ callback(event || {}); });
        }
    };
}

var rafQueue = [];
var windowTarget = createEventTarget();
windowTarget.window = windowTarget;
windowTarget.innerWidth = 2560;
windowTarget.innerHeight = 1080;
windowTarget.devicePixelRatio = 1;
windowTarget.requestAnimationFrame = function(callback){ rafQueue.push(callback); };
windowTarget.setTimeout = function(callback){ rafQueue.push(callback); };
windowTarget.console = console;
var visualViewport = createEventTarget();
windowTarget.visualViewport = visualViewport;

var documentTarget = createEventTarget();
documentTarget.getElementById = function(){ return { style:{}, onclick:null }; };
documentTarget.documentElement = {
    requestFullscreen: function(){
        documentTarget.fullscreenElement = documentTarget.documentElement;
        return Promise.resolve();
    }
};
documentTarget.exitFullscreen = function(){
    documentTarget.fullscreenElement = null;
    return Promise.resolve();
};

var context = windowTarget;
context.document = documentTarget;
context.PIXI = {};
vm.createContext(context);
vm.runInContext(fs.readFileSync('js/core/Viewport.js','utf8'), context);
vm.runInContext(fs.readFileSync('js/core/Game.js','utf8'), context);

var resizeCount = 0;
context.Game.resizeViewport = function(){ resizeCount++; };
context.Game.attachDisplayListeners();

// Multiple notifications in the same frame should cause only one viewport pass.
windowTarget.dispatch('resize');
windowTarget.dispatch('orientationchange');
documentTarget.dispatch('fullscreenchange');
visualViewport.dispatch('resize');
assert(rafQueue.length === 1, 'resize notifications were not coalesced to one animation frame');
rafQueue.shift()();
assert(resizeCount === 1, 'coalesced viewport callback should run exactly once');

// Fullscreen keyboard shortcuts must work only from a non-editable user target.
var prevented = 0;
windowTarget.dispatch('keydown', {
    key:'f', target:{tagName:'BODY'}, preventDefault:function(){ prevented++; }
});
assert(prevented === 1, 'F shortcut should be handled');
assert(documentTarget.fullscreenElement === documentTarget.documentElement,
    'F shortcut should request document fullscreen');

windowTarget.dispatch('keydown', {
    key:'f', target:{tagName:'INPUT'}, preventDefault:function(){ prevented++; }
});
assert(prevented === 1, 'F shortcut should not steal keystrokes from input fields');

windowTarget.dispatch('keydown', {
    key:'Enter', altKey:true, target:{tagName:'BODY'}, preventDefault:function(){ prevented++; }
});
assert(prevented === 2, 'Alt+Enter should toggle fullscreen');
assert(documentTarget.fullscreenElement === null, 'Alt+Enter should exit active fullscreen');

// Detach must remove the window and document listeners, including key handler.
context.Game.detachDisplayListeners();
assert((windowTarget.listeners.resize || []).length === 0, 'resize listener was not removed');
assert((documentTarget.listeners.fullscreenchange || []).length === 0, 'fullscreen listener was not removed');
assert((windowTarget.listeners.keydown || []).length === 0, 'fullscreen key handler was not removed');

console.log('Adaptive display lifecycle tests passed: resize coalescing, orientation/fullscreen/visualViewport notifications, F and Alt+Enter controls, editable-target guard, and listener cleanup.');
