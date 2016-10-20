Game.addToManifest({
    gun: "sprites/peeps/gun.json",

    gun_cock: "sounds/gun_cock.mp3"
});

/****

Same. Except....
A GUN.

Gun MC:
00-17: pulling it out.

****/

function EvilHatPeep(scene){

    var self = this;
    Peep.apply(self, [scene]);
    self._CLASS_ = "EvilHatPeep";

    // Add the body & GUN sprites
    self.gunMC = self.addMovieClip("gun");
    self.gunMC.gotoAndStop(0);
    self.bodyMC = self.addMovieClip("hatguy");
    self.bodyMC.gotoAndStop(0);

    // Position.
    self.loop = false;
    self.x = -300;
    //self.x = 300; // HAAAACK
    self.y = 470;
    self.direction = 0;

    // WHO TO KILL
    self.victim = null; // plz tell this peep.
    self.freezeEveryone = null; // plz tell this peep.
    self.bang = null; // plz tell this peep. 

    // Goes through the spot...
    var doubles = 0;
    var MODE = 0;
    MODE_WALK = 0;
    MODE_GUN = 1;
    self.goThroughSpots = true;
    self.callbacks.update = function(){

        if(self.x>420 && !self.isMurdering){
            self.itsMurderTime();
        }

        // Animate on triples.
        doubles = (doubles+1)%3;
        var gun = self.gunMC;
        var frame = gun.currentFrame;
        if(doubles==0){
            switch(MODE){
                case MODE_GUN:
                    if(frame<17) gun.gotoAndStop(frame+1);
                    break;
            }
        }

        // SPEED ONLY WHEN SEEN
        if(MODE==MODE_WALK){
            self.speed = scene.director.isWatchingTV ? 0 : 1.25;
        }

    };

    // WEIRD WALK
    self.walkAnim = function(){

        // Hop & flip
        self.hop += self.speed/40;
        if(self.hop>1) self.hop--;
        self.flip = (self.vel.x<0) ? -1 : 1;

        // Hop up & down
        var t = self.hop*Math.TAU;
        var g = self.graphics;
        g.rotation = Math.sin(t)*0.2;
        g.pivot.y = Math.abs(Math.sin(t))*7;

    };
    // WOBBLE IN PLACE
    /*self.standAnim = function(){

        // Hop & flip
        self.hop += self.speed/200;
        if(self.hop>1) self.hop--;

        // Hop up & down
        var t = self.hop*Math.TAU;
        var g = self.graphics;
        g.rotation = Math.sin(t)*0.035;
        g.pivot.y = 0;

    };*/

    // IT'S MURDER TIME
    self.isMurdering = false;
    self.hasGunOut = false;
    self.pauseAnimsWhenNotWatching = true;
    self.itsMurderTime = function(){

        // STOP.
        self.isMurdering = true;
        self.stopWalking();
        self.bounce = 1.2;

        // Half Beat.
        // Happy Smiles at you.
        self.setTimeout(function(){
            self.victim.smile();
        },_s(0.5));
        
        // +1.5 beats.
        // Pull out gun. Happy frowns.
        // AND EVERYONE FREEZES.
        self.setTimeout(function(){
            self.hasGunOut = true;
            Game.sounds.bg_park.stop(); // STAHP.
            Game.sounds.gun_cock.play(); // SOUND
            MODE = MODE_GUN;
        },_s(0.5+1.5));
        self.setTimeout(function(){
            self.freezeEveryone();
            self.victim.frown();
        },_s(0.5+1.5+0.5));

        // +4 beats.
        // BANG.
        self.setTimeout(function(){
            Game.sounds.gunshot.play();
            self.bang();
        },_s(0.5+1.5+4.0));

    }

}