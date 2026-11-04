// Run with: node tests/adaptive-gameplay.test.js
// Focused regression checks for expanded-world placement and active Director zooms.
var fs = require('fs');
var vm = require('vm');

function assert(condition, message){
    if(!condition) throw new Error(message);
}
function near(actual, expected, message){
    if(Math.abs(actual-expected) > 0.000001){
        throw new Error(message + ': expected ' + expected + ', got ' + actual);
    }
}

// Game's non-World anchors must follow changes in the centered base offset
// without resetting their current positions.
var resizeListeners = {};
var gameContext = {
    innerWidth: 1920,
    innerHeight: 1080,
    devicePixelRatio: 1,
    addEventListener: function(name, callback){ resizeListeners[name] = callback; },
    removeEventListener: function(name){ delete resizeListeners[name]; },
    document: {
        getElementById: function(){ return { style: {}, onclick: null }; },
        querySelector: function(){ return { appendChild: function(){} }; },
        body: { appendChild: function(){} }
    },
    PIXI: { Rectangle: function(x,y,width,height){ this.x=x; this.y=y; this.width=width; this.height=height; } }
};
gameContext.window = gameContext;
vm.createContext(gameContext);
vm.runInContext(fs.readFileSync('js/core/Viewport.js', 'utf8'), gameContext);
vm.runInContext(fs.readFileSync('js/core/Game.js', 'utf8'), gameContext);
var Game = gameContext.Game;
Game.resizeViewport(1920,1080);
var avoidSpot = Game.anchorBasePosition({ radius: 150 }, 480, 430);
near(avoidSpot.x, 480, 'base anchor initial x');
near(avoidSpot.y, 430, 'base anchor initial y');
// Simulate normal movement after creation; resize should preserve that motion.
avoidSpot.x += 7;
avoidSpot.y -= 3;
Game.resizeViewport(2560,1080);
near(avoidSpot.x, 647, 'base anchor moves by the horizontal base-offset delta');
near(avoidSpot.y, 427, 'base anchor preserves vertical motion');
Game.unanchorBasePosition(avoidSpot);
Game.resizeViewport(1920,1200);
near(avoidSpot.x, 647, 'unanchored point does not move on later resize');

// World props and a zoomed world pivot must move by the same base-offset delta.
var worldContext = {
    Game: {
        width: 1280,
        height: 540,
        getBaseOffset: function(){ return {x:160,y:0}; },
        createViewportFill: function(){ return {}; },
        addToManifest: function(){ }
    },
    PIXI: {
        Container: function(){
            this.children=[];
            this.x=0; this.y=0;
            this.pivot={x:0,y:0};
            this.scale={x:2,y:2};
            this.position=this;
            this.addChild=function(child){ this.children.push(child); };
        }
    },
    MakeSprite: function(){ return { position: {x:0,y:0} }; }
};
vm.createContext(worldContext);
vm.runInContext(fs.readFileSync('js/game/World.js','utf8'), worldContext);
var fakeScene = { graphics: { addChild: function(){} } };
var world = new worldContext.World(fakeScene);
var actor = {x:300,y:240,graphics:{}};
world.props.push(actor);
world.graphics.pivot.x=500;
world.graphics.pivot.y=260;
world.onViewportResize(
    {extraLeft:0,extraTop:0},
    {extraLeft:160,extraTop:0}
);
near(actor.x,460,'existing world prop follows expanded base composition');
near(actor.y,240,'existing world prop preserves y');
near(world.graphics.pivot.x,660,'zoom pivot follows translated world content');
near(world.graphics.pivot.y,260,'zoom pivot preserves y');
near(world.graphics.x,640,'world renderer origin follows new viewport center');

// Director viewport transitions must rebase seamlessly and recalculate their
// target when the viewport changes during an active zoom.
var tweenRecords=[];
var directorContext = {
    Game: { width:960, height:540, addToManifest:function(){} },
    Tween_get: function(target){
        var record={target:target, callback:null};
        tweenRecords.push(record);
        return {
            to: function(){ return this; },
            call: function(callback){ record.callback=callback; return this; }
        };
    },
    _s: function(value){ return value; },
    BEAT: 1,
    Ease: { cubicInOut:function(){}, circInOut:function(){} },
    isFinite: isFinite,
    Math: Math
};
vm.createContext(directorContext);
vm.runInContext(fs.readFileSync('js/game/Director.js','utf8'), directorContext);
var worldGraphics={pivot:{x:480,y:270},scale:{x:1,y:1},x:480,y:270};
var tv={x:480,y:350,offset:{x:0,y:-113.5,scale:8}};
var director=new directorContext.Director({world:{graphics:worldGraphics},camera:{},tv:tv});
director.tweenViewportTo(function(){
    return {x:directorContext.Game.width/2+80,y:directorContext.Game.height/2+20,scale:2};
},100,directorContext.Ease.cubicInOut);
var tween=tweenRecords[tweenRecords.length-1].target;
tween.t=0.5;
director.update();
var before={x:worldGraphics.pivot.x,y:worldGraphics.pivot.y,scale:worldGraphics.scale.x};
// Simulate the World resize phase before Director.onViewportResize.
directorContext.Game.width=1280;
directorContext.Game.height=600;
worldGraphics.x=640;
worldGraphics.y=300;
worldGraphics.pivot.x += 160;
worldGraphics.pivot.y += 30;
director.onViewportResize(
    {extraLeft:0,extraTop:0},
    {extraLeft:160,extraTop:30}
);
director.update();
near(worldGraphics.pivot.x,before.x+160,'Director rebase avoids x jump after resize');
near(worldGraphics.pivot.y,before.y+30,'Director rebase avoids y jump after resize');
near(worldGraphics.scale.x,before.scale,'Director rebase avoids zoom-scale jump');
tween.t=1;
director.update();
near(worldGraphics.pivot.x,720,'Director resolves dynamic target x after resize');
near(worldGraphics.pivot.y,320,'Director resolves dynamic target y after resize');
near(worldGraphics.scale.x,2,'Director reaches requested zoom scale');
tweenRecords[tweenRecords.length-1].callback();
assert(director._viewportTweenState === null,'Director clears completed viewport tween state');


// Camera photo-to-center movement must likewise rebase when the viewport changes.
var cameraTweens=[];
function Container(){
    this.children=[]; this.x=0; this.y=0; this.visible=true;
    this.scale={x:1,y:1}; this.anchor={x:0,y:0};
    this.addChild=function(child){ this.children.push(child); };
    this.removeChildren=function(){ this.children=[]; };
}
function Sprite(texture){ this.texture=texture; this.scale={x:1,y:1}; this.anchor={x:0,y:0}; this.alpha=1; }
var cameraContext={
    Game: {
        baseWidth:960, baseHeight:540, width:960, height:540,
        renderer:{},
        stage:{},
        sounds:{cam_snap:{play:function(){}}},
        addToManifest:function(){},
        toWorldPosition:function(point){ return point; }
    },
    PIXI:{
        Container:Container,
        Sprite:Sprite,
        RenderTexture:{create:function(width,height){ return {width:width,height:height}; }},
        loader:{resources:{
            cam_flash:{texture:{}}, cam_frame:{texture:{}}, cam_instructions:{texture:{}}
        }}
    },
    Tween_get:function(target){
        var record={target:target,callback:null};
        cameraTweens.push(record);
        return {
            wait:function(){ return this; },
            to:function(){ return this; },
            call:function(callback){ record.callback=callback; return this; }
        };
    },
    _s:function(value){ return value; },
    BEAT:1,
    Ease:{circInOut:function(){},quadInOut:function(){}}
};
vm.createContext(cameraContext);
vm.runInContext(fs.readFileSync('js/game/Camera.js','utf8'),cameraContext);
var cameraScene={graphics:new cameraContext.PIXI.Container(),world:{graphics:new cameraContext.PIXI.Container()}};
var camera=new cameraContext.Camera(cameraScene,{noIntro:true});
camera.frozen=true;
camera.moveToViewportCenter(100,cameraContext.Ease.circInOut);
var cameraTween=cameraTweens[cameraTweens.length-1].target;
cameraTween.t=0.5;
camera.update();
var cameraBefore={x:camera.graphics.x,y:camera.graphics.y,scale:camera.graphics.scale.x};
cameraContext.Game.width=1280;
cameraContext.Game.height=540;
camera.onViewportResize(
    {extraLeft:0,extraTop:0},
    {extraLeft:160,extraTop:0}
);
camera.update();
near(camera.graphics.x,cameraBefore.x+160,'camera move rebase preserves x continuity');
near(camera.graphics.y,cameraBefore.y,'camera move rebase preserves y continuity');
near(camera.graphics.scale.x,cameraBefore.scale,'camera move rebase preserves scale continuity');
cameraTween.t=1;
camera.update();
near(camera.graphics.x,640,'camera photo animation reaches the new viewport center x');
near(camera.graphics.y,270,'camera photo animation reaches the new viewport center y');
near(camera.graphics.scale.x,1280/240,'camera photo animation covers the expanded viewport');
cameraTweens[cameraTweens.length-1].callback();
assert(camera._moveToCenterState === null,'camera clears completed center tween state');

// Guard the fixed-coordinate integrations that are easy to miss in manual review.
var sourceChecks=[
    ['js/peeps/HappyWeirdoPeep.js','baseToWorldPosition(115, 170)'],
    ['js/peeps/CrazyPeep.js','baseToWorldPosition(1200, 330)'],
    ['js/peeps/EvilHatPeep.js','baseToWorldPosition(-300, 470)'],
    ['js/peeps/PanicPeep.js','baseToWorldPosition(635, 200)'],
    ['js/peeps/LoverPeep.js','baseToWorldPosition(1000, 580)'],
    ['js/peeps/ProtestAnim.js','baseToWorldPosition(650, 200)'],
    ['js/peeps/HelpingAnim.js','baseToWorldPosition(180, 170)'],
    ['js/game/ScreenZoomOut.js','-offsetX + baseOffset.x']
];
sourceChecks.forEach(function(check){
    assert(fs.readFileSync(check[0],'utf8').indexOf(check[1])>=0,
        'missing adaptive coordinate mapping in '+check[0]);
});

console.log('Adaptive gameplay tests passed: anchored avoid zones, world prop translation, zoom-pivot translation, resize-safe Director and camera transitions, and fixed-coordinate integrations.');
