// Run with: node tests/startup-compatibility.test.js
// Validates the downloaded source's default translation selection and Pixi 4 RT API.
var fs = require('fs');
var vm = require('vm');
var assert = function(condition, message){ if(!condition) throw new Error(message); };
var context = {};
vm.createContext(context);
vm.runInContext(fs.readFileSync('js/textStrings.js', 'utf8'), context);
assert(context.textStrings, 'active textStrings dictionary was not created');
assert(context.textStrings === context.textStrings_EN, 'standalone source must default to English');
['manifesto', 'playingTime', 'warning', 'playButton', 'niceHat', 'crazySquareAttacks'].forEach(function(key){
    assert(context.textStrings[key] !== undefined, 'English text key is missing: ' + key);
});
assert(context.textStrings_DE && context.textStrings_FA && context.textStrings_TR, 'existing translations should remain available');
// Act_II.js reads the manifesto at script evaluation time; evaluate it as the reported browser does.
vm.runInContext(fs.readFileSync('js/scenes/Act_II.js', 'utf8'), context);
assert(context._manifesto && context._manifesto.length >= 1, 'Act II manifesto was not initialized');
var camera = fs.readFileSync('js/game/Camera.js', 'utf8');
var preloader = fs.readFileSync('js/scenes/Scene_Preloader.js', 'utf8');
var meta = fs.readFileSync('js/scenes/Scene_Meta.js', 'utf8');
[camera, preloader, meta].forEach(function(source){
    assert(source.indexOf('new PIXI.RenderTexture(') === -1, 'deprecated RenderTexture constructor remains');
    assert(source.indexOf('PIXI.RenderTexture.create(') >= 0, 'PixiJS 4 RenderTexture.create API missing');
});
console.log('Startup compatibility tests passed: English dictionary selected, translations retained, PixiJS 4 RenderTexture API used.');
