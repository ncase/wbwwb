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
    self.world.layers.bg.addChild(MakeSprite("bg_shade"));
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
    self.stream.width = Game.width/8;
    self.stream.height = Game.height/8;
    self.stream.x = Game.width/2 - self.stream.width/2 - 2; // hack
    self.stream.y = Game.height/2 - self.stream.height/2 + 2; // hack
    self.world.layers.bg.addChild(self.stream);

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

    // Candlelights
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
        var pos = candlePositions[i];
        var candle = new Candlelight(pos);
        self.world.addBG(candle);
    }

    // The lovers
    self.world.addProp(new LoversWatching("circle"));
    self.world.addProp(new LoversWatching("square"));

    // The 3 crickets
    var cricketPositions = [
        [400, 353],
        [420, 370],
        [450, 380]
    ];
    for(var i=0;i<cricketPositions.length;i++){
        var pos = cricketPositions[i];
        var cricket = new Cricket(self);
        cricket.x = pos[0];
        cricket.y = pos[1];
        cricket.mc.gotoAndStop(1);
        self.world.addProp(cricket);
    }

    /////////////
    // FADE IN //
    /////////////

    var blackout = MakeSprite("blackout");
    Game.stage.addChild(blackout);
    Tween_get(blackout).to({alpha:0}, _s(BEAT), Ease.quadInOut).call(function(){
        Game.stage.removeChild(blackout);
    });

}