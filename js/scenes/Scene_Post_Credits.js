Game.addToManifest({
    bg_shade: "sprites/bg_shade.png",
    bg_nighttime: "sounds/bg_nighttime.mp3"
});

/************************************

THE GAME SCENE. THE BIG 'UN.

ACT I - Teaching controls, showing main feedback loop
ACT II - Crazy, Nervous, Snobby, Angry escalation...
ACT III - Protest! (what gameplay here???)
ACT IV - MURDER AND VIOLENCE AND AHHHHHH. #BeScaredBeAngry

(different scene...)
ACT V - Post-credits peace

*************************************/

function Scene_Post_Credits(){

	var self = this;
	Scene.call(self);

    // HACK: Background Ambience
    /*var ambience = Game.sounds.bg_nighttime;
    ambience.loop(true);
    ambience.play();*/

	////////////
	// SET UP //
	////////////

    // Graphics!
    var g = new PIXI.Container();
    self.graphics = g;
    Game.stage.addChild(g);

	// Set Up Everything
    self.world = new World(self,{
        bg: "bg_dark"
    });
    var shade = MakeSprite("bg_shade");
    var baseOffset = Game.getBaseOffset();
    shade.x = baseOffset.x;
    shade.y = baseOffset.y;
    self.world.layers.bg.addChild(shade);
    self.camera = new Camera(self,{
        noIntro: true,
        streaming: true,
        onTakePhoto: function(){
            //if(self.camera.isOverTV()){
            Game.sounds.bg_nighttime.stop();
            Game.sceneManager.gotoScene("Post_Post_Credits");
            //}
        }
    });
    self.camera.x = Game.width;
    self.camera.y = Game.height;

    // Put a SPRITE RIGHT IN THE BG
    self.stream = new PIXI.Sprite();
    self.stream.width = Game.baseWidth/8;
    self.stream.height = Game.baseHeight/8;
    self.stream.x = Game.width/2 - self.stream.width/2 - 2; // hack
    self.stream.y = Game.height/2 - self.stream.height/2 + 2; // hack
    self.world.layers.bg.addChild(self.stream);

    self.onViewportResize = function(previousMetrics, metrics){
        var offset = Game.getBaseOffset();
        shade.x = offset.x;
        shade.y = offset.y;
        self.stream.width = Game.baseWidth/8;
        self.stream.height = Game.baseHeight/8;
        self.stream.x = Game.width/2 - self.stream.width/2 - 2;
        self.stream.y = Game.height/2 - self.stream.height/2 + 2;
        // Static post-credit props retain their original relationship to the TV.
        if(previousMetrics){
            var dx = offset.x - previousMetrics.extraLeft;
            var dy = offset.y - previousMetrics.extraTop;
            fixedProps.forEach(function(prop){
                // Crickets are World props and are already translated by
                // World.onViewportResize; only background props and graphics-
                // only decorations need this scene-level adjustment.
                if(prop.prop) return;
                prop.x += dx;
                prop.y += dy;
                if(prop.graphics){
                    prop.graphics.x = prop.x;
                    prop.graphics.y = prop.y;
                }
            });
        }
    };

    // UPDATE
    self.update = function(){

        self.world.update();
        self.camera.update();

        // THE STREAM
        self.stream.texture = self.camera.getTexture();

    };

    //////////////////////
    // EVEN MORE SET UP //
    //////////////////////

    // Candlelights and other set dressing were authored in the original
    // 960x540 coordinate space; move them with the centered base composition.
    var fixedProps = [];
    var candlePositions =[
        [468.6,276.6],
        [535.7,281.6],
        [679.7,279.1],
        [612,281.6],
        [490.1,314.1],
        [421.8,309],
        [363.1,301.1],
        [786.9,304.4],
        [726.1,310.5],
        [656.9,309.5],
        [869.8,350.3],
        [820,373.5],
        [768.2,382.8],
        [698.4,389],
        [464.6,386.1],
        [396.3,382.3],
        [339.6,370.1],
        [294.3,350.3]
    ];
    for(var i=0;i<candlePositions.length;i++){
        var pos = Game.baseToWorldPosition(candlePositions[i][0], candlePositions[i][1]);
        var candle = new Candlelight([pos.x, pos.y]);
        fixedProps.push({ x: pos.x - 100, y: pos.y - 100, graphics: candle.graphics });
        self.world.addBG(candle);
    }

    // The lovers
    var circleLover = new LoversWatching("circle");
    var squareLover = new LoversWatching("square");
    var baseOffsetForLovers = Game.getBaseOffset();
    circleLover.graphics.x += baseOffsetForLovers.x;
    circleLover.graphics.y += baseOffsetForLovers.y;
    squareLover.graphics.x += baseOffsetForLovers.x;
    squareLover.graphics.y += baseOffsetForLovers.y;
    fixedProps.push({ x: circleLover.graphics.x, y: circleLover.graphics.y, graphics: circleLover.graphics });
    fixedProps.push({ x: squareLover.graphics.x, y: squareLover.graphics.y, graphics: squareLover.graphics });
    self.world.addProp(circleLover);
    self.world.addProp(squareLover);

    // The 3 crickets
    var cricketPositions = [
        [400, 353],
        [420, 370],
        [450, 380]
    ];
    for(var i=0;i<cricketPositions.length;i++){
        var pos = Game.baseToWorldPosition(cricketPositions[i][0], cricketPositions[i][1]);
        var cricket = new Cricket(self);
        cricket.x = pos.x;
        cricket.y = pos.y;
        fixedProps.push({ x: cricket.x, y: cricket.y, prop: cricket, graphics: cricket.graphics });
        cricket.mc.gotoAndStop(1);
        self.world.addProp(cricket);
    }

    /////////////
    // FADE IN //
    /////////////

    var blackout = Game.createViewportFill(0x000000);
    Game.stage.addChild(blackout);
    Tween_get(blackout).to({alpha:0}, _s(BEAT), Ease.quadInOut).call(function(){
        Game.stage.removeChild(blackout);
    });

}