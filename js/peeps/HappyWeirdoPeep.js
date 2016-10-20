Game.addToManifest({
	happy_weirdo: "sprites/peeps/happy_weirdo.json"
});

/****

FRAMES:
00-00: happy still
01-02: blink to greet murderer
03-05: shock.

****/

function HappyWeirdoPeep(scene){

	var self = this;
	Peep.apply(self, [scene]);
    self._CLASS_ = "HappyWeirdoPeep";

    // position
    self.x = 115;
    self.y = 170;

	// MAD SPRITE
    var g = self.graphics;
    self.bodyMC = self.addMovieClip("happy_weirdo");
    self.bodyMC.anchor.x = 0.35; // not quite

    // MODE o' ANIMATION
    var MODE = -1;
    MODE_SMILE = 1;
    MODE_FROWN = 2;
    self.smile = function(){
        MODE = MODE_SMILE;
    };
    self.frown = function(){
        MODE = MODE_FROWN;
    };
    var doubles = 0;

    // WANDERING
    self.wander = 0;
    self.changeWander = function(){
        self.wander = Math.random()*0.1-0.05;
    };
    self.callbacks.update = function(){

        // Wander around
        self.direction += self.wander;
        if(Math.random()<0.05) self.changeWander();

        // STAY WITHIN GAME FRAME
        self.stayWithinRect({
            l:100, r:860, t:100, b:480
        },0.15);

        // Animation! on quadtriples.
        doubles = (doubles+1)%4;
        var body = self.bodyMC;
        var frame = body.currentFrame;
        if(doubles==0){
            switch(MODE){
                case MODE_SMILE:
                    if(frame<2) body.gotoAndStop(frame+1);
                    break;
                case MODE_FROWN:
                    if(frame<5) body.gotoAndStop(frame+1);
                    break;
            }
        }

    };
    self.callbacks.startWalking = function(){
        self.speed = 2;
    };
    self.callbacks.startWalking();

    // WEIRD WALK
    self.hop = 0;
    self._lastHop = 0.99;
    self.direction = 1;
    self.walkAnim = function(){

        // Hop & flip
        self.hop += self.speed/50;
        if(self.hop>1) self.hop--;
        self.flip = (self.vel.x<0) ? -1 : 1;

        // Hop up & down
        var t = self.hop*Math.PI;
        g.pivot.y = Math.abs(Math.sin(t))*20;
        g.rotation = 0;

        // Squash at the bottom of your cycle
        if(self._lastHop>0.9 && self.hop<=0.1) self.bounce=1.2;

    };

    // WOBBLE IN PLACE
    self.standAnim = function(){

        // Hop & flip
        self.hop += self.speed/150;
        if(self.hop>1) self.hop--;
        self.flip = (self.vel.x<0) ? -1 : 1;
        
        // Wobble!
        if(MODE!=MODE_FROWN){
            var t = self.hop*Math.TAU;
            g.rotation = Math.sin(t)*0.1;
            g.pivot.y = 0;
        }

    };

    // Prepare to get murdered
    self.prepareForMurder = function(){

        // Wobble in place
        self.stopWalking();
        self.x = 540;
        self.y = 470;
        self.vel.x = -1;
        self.vel.y = 0;
        self.flip = -1;

        // AVOID SPOT. BAD STUFF GON' HAPPEN.
        var spot = {
            x: 480,
            y: 430,
            radius: 150
        };
        scene.avoidSpots.push(spot);

    };

    // GET MURDERED

}