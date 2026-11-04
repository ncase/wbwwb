// Run with: node tests/viewport.test.js
var fs = require('fs');
var vm = require('vm');
var context = { window: {} };
vm.runInNewContext(fs.readFileSync('js/core/Viewport.js', 'utf8'), context);
var Viewport = context.window.Viewport;
var viewport = new Viewport(960, 540);
function near(actual, expected, label){
    if(Math.abs(actual - expected) > 0.000001){
        throw new Error(label + ': expected ' + expected + ', got ' + actual);
    }
}
var cases = [
    {w:1920,h:1080,worldW:960,worldH:540,scale:2},
    {w:2560,h:1080,worldW:1280,worldH:540,scale:2},
    {w:3840,h:1080,worldW:1920,worldH:540,scale:2},
    {w:1920,h:1200,worldW:960,worldH:600,scale:2},
    {w:1600,h:1200,worldW:960,worldH:720,scale:1600/960}
];
cases.forEach(function(c){
    var m = viewport.calculate(c.w,c.h);
    near(m.width,c.worldW,c.w+' world width');
    near(m.height,c.worldH,c.w+' world height');
    near(m.scale,c.scale,c.w+' scale');
    near(m.centerX,m.width/2,c.w+' centerX');
    near(m.centerY,m.height/2,c.w+' centerY');
    near(m.extraLeft,m.extraRight,c.w+' horizontal symmetry');
    near(m.extraTop,m.extraBottom,c.w+' vertical symmetry');
});
var invalid = false;
try { viewport.calculate(0, 1080); } catch(e) { invalid = true; }
if(!invalid) throw new Error('zero width should be rejected');
console.log('Viewport tests passed: 5 aspect/resolution cases + invalid dimensions.');

// The original 960x540 composition must remain centered in expanded worlds.
var ultra = viewport.calculate(2560,1080);
var wideBase = viewport.baseToWorldPosition(0,0,ultra);
near(wideBase.x,160,'ultrawide base x offset');
near(wideBase.y,0,'ultrawide base y offset');
var basePoint = viewport.worldToBasePosition(700,200,ultra);
near(basePoint.x,540,'world-to-base x');
near(basePoint.y,200,'world-to-base y');
var screenPoint = viewport.screenToBasePosition(1400,400,ultra);
near(screenPoint.x,540,'screen-to-base x');
near(screenPoint.y,200,'screen-to-base y');
var tall = viewport.calculate(1600,1200);
var tallBase = viewport.baseToWorldPosition(0,0,tall);
near(tallBase.x,0,'tall base x offset');
near(tallBase.y,90,'tall base y offset');
