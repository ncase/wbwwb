/************************************

THE GAME SCENE. THE BIG 'UN.

ACT I - Teaching controls, showing main feedback loop
ACT II - Crazed Square, Nervous Circle, Snobby Square...
ACT III - Angry escalation! And lovers protest!
ACT IV - MURDER AND VIOLENCE AND AHHHHHH. #BeScaredBeAngry

(different scene...)
ACT V - Post-credits peace

*************************************/

function Scene_Game(){

	var self = this;
	Scene.call(self);

	////////////
	// SET UP //
	////////////

    // Graphics!
    var g = new PIXI.Container();
    self.graphics = g;
    Game.stage.addChild(g);

	// Set Up Everything
    self.world = new World(self);
    self.camera = new Camera(self);
    self.director = new Director(self);
    self.tv = new TV(self);
    self.world.addProp(self.tv);

    // Special effects!
    self.scale = 1;
    self.x = self.y = self.offX = self.offY = 0;
    self.shaker = new ScreenShake(self);
    self.zoomer = new ScreenZoomOut(self);

    // Avoid these spots.
    self.avoidSpots = [];

    // UPDATE
    self.update = function(){
        
        self.world.update();
        self.camera.update();
        self.director.update();

        // This order is important
        self.zoomer.update();
        self.shaker.update();
        g.scale.x = g.scale.y = self.scale;
        g.x = self.x + self.offX;
        g.y = self.y + self.offY;
        self.zoomer.fixLaptop(); // hack.

        // TOTALLY A HACK
        var ratio = self.zoomer.timer/self.zoomer.fullTimer;
        ratio = (1-ratio)/1;
        self.shaker.baseAlpha = 0.15 + ratio*0.45;

    };

	// TO IMPLEMENT
	self.kill = function(){};

    // Going to a Stage
    self.go = function(sceneFunc){
        sceneFunc(self);
        self.update();
    };

    /////////////
    // FADE IN //
    /////////////

    var blackout = MakeSprite("blackout");
    Game.stage.addChild(blackout);
    Tween_get(blackout).to({alpha:0}, _s(BEAT), Ease.quadInOut).call(function(){
        Game.stage.removeChild(blackout);
    });

    ////////////
    // STAGES //
    ////////////

    Stage_Start(self);
    Stage_Hat(self);
    //Stage_Lovers(self);
    //Stage_Screamer(self, true);
    //Stage_Nervous(self, true);
    //Stage_Snobby(self, true);
    //Stage_Angry_Escalation(self, true);
    //Stage_Evil(self, true);

}