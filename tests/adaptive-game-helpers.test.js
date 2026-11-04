// Run with: node tests/adaptive-game-helpers.test.js
var fs = require('fs');
var vm = require('vm');

function FakeContainer(){
    this.children = [];
    this.scale = { x: 1, y: 1 };
    this.x = 0;
    this.y = 0;
}
FakeContainer.prototype.addChild = function(child){ this.children.push(child); };

function FakeGraphics(){
    this.commands = [];
    this.alpha = 1;
}
FakeGraphics.prototype.clear = function(){ this.commands = []; };
FakeGraphics.prototype.beginFill = function(color, alpha){ this.commands.push(['beginFill', color, alpha]); };
FakeGraphics.prototype.drawRect = function(x,y,w,h){ this.commands.push(['drawRect',x,y,w,h]); };
FakeGraphics.prototype.endFill = function(){ this.commands.push(['endFill']); };

var context = {
    console: console,
    document: { getElementById: function(){ return {}; } },
    PIXI: {
        Container: FakeContainer,
        Graphics: FakeGraphics,
        Rectangle: function(x,y,w,h){ this.x=x; this.y=y; this.width=w; this.height=h; }
    }
};
context.window = context;
context.innerWidth = 2560;
context.innerHeight = 1080;
context.devicePixelRatio = 1;
vm.createContext(context);
vm.runInContext(fs.readFileSync('js/core/Viewport.js','utf8'), context);
vm.runInContext(fs.readFileSync('js/core/Game.js','utf8'), context);

var Game = context.Game;
Game.resizeViewport(2560,1080);
var layout = Game.createBaseLayout();
var fill = Game.createViewportFill(0xa5a5a5);
function near(actual, expected, message){
    if(Math.abs(actual-expected)>1e-6) throw new Error(message+': expected '+expected+', got '+actual);
}
near(layout.x,160,'base layout horizontal centering');
near(layout.y,0,'base layout vertical centering');
near(fill.commands[1][1],0,'fill rectangle x');
near(fill.commands[1][2],0,'fill rectangle y');
near(fill.commands[1][3],1280,'fill rectangle world width');
near(fill.commands[1][4],540,'fill rectangle world height');
var base = Game.toBasePosition({x:1400,y:400});
near(base.x,540,'pointer-to-base x');
near(base.y,200,'pointer-to-base y');

Game.resizeViewport(1920,1200);
near(layout.x,0,'layout horizontal offset after resize');
near(layout.y,30,'layout vertical offset after resize');
near(fill.commands[1][3],960,'resized fill world width');
near(fill.commands[1][4],600,'resized fill world height');
var baseAfterResize = Game.toBasePosition({x:960,y:460});
near(baseAfterResize.x,480,'resized pointer-to-base x');
near(baseAfterResize.y,200,'resized pointer-to-base y');

console.log('Adaptive game helper tests passed: centered base layouts, viewport fills, and pointer conversion across resize.');
