Game.addToManifest({
    face_angry: "sprites/peeps/face_angry.json",
    body_red: "sprites/peeps/body_red.json",

    shout: "sounds/shout.mp3"
});

/****

FRAMES:
00-05: frown at screen
06-10: anger awayyyyyy!
11-25: SHOUT! // loop back to 10. (>=20, no, not screaming)

****/

function AngryPeep(scene, type){

    var self = this;
    Peep.apply(self, [scene]);
    self._CLASS_ = "AngryPeep";

    // Add the body & face sprites
    var g = self.graphics;
    self.bodyMC = self.addMovieClip("body");
    self.bodyRedMC = self.addMovieClip("body_red");
    self.bodyRedMC.alpha = 0;
    self.faceMC = self.addMovieClip("face_angry");
    self.faceMC.anchor.x = 0.333333333;
    self.faceMC.gotoAndStop(0);

    // Set Type: Am I a circle or square?
    self.type = "???";
    self.setType = function(type){
        self.type = type;
        self.bodyMC.gotoAndStop((type=="circle") ? 0 : 1);
        self.bodyRedMC.gotoAndStop(self.bodyMC.currentFrame);
    };
    self.setType(type);

    // IS SHOUTING?
    self.isShouting = false;

    // Animate on triples
    var doubles = 0;
    var MODE = -1;
    var MODE_STARE = 0;
    var MODE_BLINK = 1;
    var MODE_SHOUT = 2;
    self.gracePeriod = -1;

    // HACK
    self.HACK_JUMPSTART = function(){
        MODE = MODE_BLINK;
    };

    // WANDERING
    self.wander = 0;
    self.changeWander = function(){
        self.wander = Math.random()*0.1-0.05;
    };

    self.callbacks.update = function(){
        
        // Animate on doubles! ...or... TRIPLES?
        doubles = (doubles+1)%3;

        // Wander around
        self.direction += self.wander;
        if(Math.random()<0.05) self.changeWander();
        self.stayWithinRect({
            l:100, r:860, t:100, b:480
        },0.15);

        // FRAMES: MANUALLY
        var face = self.faceMC;
        var frame = face.currentFrame;
        if(doubles==0){
            switch(MODE){
                case MODE_STARE:
                    if(frame<5) face.gotoAndStop(frame+1);
                    break;
                case MODE_BLINK:
                    if(frame<10){
                        face.gotoAndStop(frame+1);
                    }
                    break;
                case MODE_SHOUT:
                    if(frame<25){
                        face.gotoAndStop(frame+1);
                    }else{
                        face.gotoAndStop(10);
                        MODE = MODE_BLINK;
                        self.isShouting = false;
                        self.startWalking();
                    }
                    break;
            }
        }

        if(frame>=10){
            self.bodyRedMC.alpha = self.bodyRedMC.alpha*0.9 + 1*0.1;
        }

        ///////////////////////////
        ///////////////////////////
        ///////////////////////////

        // Less red bod...
        // self.bodyRedMC.alpha *= 0.985;

        // Scream at THE OPPOSITE TYPE
        // Grace period... AND IF SCENE ALLOWS IT.
        if(self.gracePeriod<=0 && !scene.noYellingYet){
            var opposite = (self.type=="circle") ? "square" : "circle";
            var closeTo = self.touchingPeeps(90, function(peep){
                return(!peep.offended && peep.isWalking && peep.type==opposite);
            });
            if(self.isWalking && closeTo.length>0){

                var other = closeTo[0];

                // Bounce TOWARDS
                self.flip = (other.x>self.x) ? 1 : -1;
                self.bounce = 1.7;
                self.vel.y = 0;
                self.vel.x = 3*self.flip;

                // BE SHOCKED
                MODE = MODE_SHOUT;

                self.isWalking = false;
                self.isShouting = true;

                // AHHHHH
                var peeps = scene.world.peeps;
                var angry = peeps.filter(function(peep){
                    return(peep._CLASS_=="AngryPeep");
                });
                var angryNum = angry.length;
                var volume;
                if(angryNum<5) volume=1;
                else if(angryNum<10) volume=0.70;
                else if(angryNum<15) volume=0.42;
                else volume=0.27;
                var shout = Game.sounds.shout;
                shout.volume(volume);
                shout.play(); 

                // MAKE BOD REDDER
                // self.bodyRedMC.alpha = 1;

                // GRACE...
                self.gracePeriod = _s(2.5);

                // If the closeTo is ANOTHER ANGRY ONE.
                // They get confused!
                if(other._CLASS_=="NormalPeep"){
                    if(other.shocked) return;
                    other.vel.x = self.flip*5;
                    other.flip = -1*self.flip;
                    other.beShocked(self);
                }else{
                    // nothing...?
                }

            }
        }else{
            self.gracePeriod--;
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
        g.pivot.y = Math.abs(Math.sin(t))*15;
        g.rotation = 0;

        // Squash at the bottom of your cycle
        if(self._lastHop<0.5 && self.hop>=0.5) self.bounce = 1.2;
        if(self._lastHop>0.9 && self.hop<=0.1) self.bounce = 1.2;

    };
    self.callbacks.startWalking = function(){
        self.speed = 1.7;
    };
    self.callbacks.startWalking();

    // AT FIRST...
    self.watchTV = function(){

        self.clearAnims(); // just in case...
        
        // 0) Stop & look
        var tv = scene.tv;
        self.stopWalking(true);
        self.flip = (tv.x>self.x) ? 1 : -1;
        var OFFSET = (Math.abs(self.x-tv.x)-60)/100;
        var WAIT = Director.ZOOM_OUT_1_TIME + Director.SEE_VIEWERS_TIME;

        // 1) Become frowny
        self.setTimeout(function(){
            
            self.bounce = 1.6;
            MODE = MODE_STARE;
            
            // SQUEAK
            Game.sounds.squeak.play();

        },_s(BEAT*2+OFFSET));

        // 2) Blink...
        self.setTimeout(function(){
            self.bounce = 1.3;
            MODE = MODE_BLINK;
        },_s(WAIT+OFFSET));

        // 3) And go on.
        self.setTimeout(function(){
            // self.bounce = 1.2;
            self.startWalking();
        },_s(WAIT+1+OFFSET));

    };

}