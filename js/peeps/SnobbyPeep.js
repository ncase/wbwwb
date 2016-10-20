Game.addToManifest({

    face_snobby: "sprites/peeps/face_snobby.json",
    face_snobby_hmph: "sprites/peeps/face_snobby_hmph.json",

    peep_huh: "sounds/peep_huh.mp3",
    peep_hmph: "sounds/peep_hmph.mp3"

});

/****

FRAMES:
00-03: disbelief at screen
04-07: grouch face (07 is resting)
09-14: SMUG 
15-20: HMPH. (LOOP HMPH at 16)
22-25: POP.
26-29: Go Away. FLIP, then go to 07.

****/

function SnobbyPeep(scene){

	var self = this;
	Peep.apply(self, [scene]);
    self._CLASS_ = "SnobbyPeep";

	// Add the body & face sprites
    self.bodyMC = self.addMovieClip("body");
    self.bodyMC.gotoAndStop(1);
    self.faceMC = self.addMovieClip("face_snobby");
    self.faceMC.anchor.y = 1.25;
    self.faceMC.gotoAndStop(0);
    self.wordMC = self.addMovieClip("face_snobby_hmph");
    self.wordMC.anchor.y = 1.25;
    self.wordMC.gotoAndStop(0);

    // IS SMUG?
    self.isSmug = false;

    // Animate on triples
    var doubles = 0;
    var MODE = -1;
    var MODE_STARE = 0;
    var MODE_BLINK = 1;
    var MODE_SMUG = 2;
    var MODE_HMPH = 3;
    var MODE_POP = 4;
    var MODE_AWAY = 5;
    self.gracePeriod = -1;

    // HACK
    self.HACK_JUMPSTART = function(){
        MODE = MODE_BLINK;
    };

    self.callbacks.update = function(){
        
        // Animate on doubles! ...or... TRIPLES?
        doubles = (doubles+1)%3;

        // stay within game frame
        self.stayWithinRect({
            l:100, r:860, t:100, b:480
        },0.05);

        // FRAMES: MANUALLY
        var face = self.faceMC;
        var frame = face.currentFrame;
        self.wordMC.scale.x = self.flip*self.DRAWING_SCALE;
        if(doubles==0){
            switch(MODE){
                case MODE_STARE:
                    if(frame<3) face.gotoAndStop(frame+1);
                    break;
                case MODE_BLINK:
                    if(frame<7) face.gotoAndStop(frame+1);
                    break;
                case MODE_SMUG:
                    if(frame<14) face.gotoAndStop(frame+1);
                    break;
                case MODE_HMPH:
                    if(frame<20){
                        frame = frame+1;
                        face.gotoAndStop(frame);
                        if(frame>=18){
                            if(self.wordMC.currentFrame==0){
                                self.wordMC.gotoAndStop(1);
                            }else{
                                var nextFrame = self.wordMC.currentFrame+1;
                                if(nextFrame>3) nextFrame=1;
                                self.wordMC.gotoAndStop(nextFrame);
                            }
                        }
                    }
                    break;
                case MODE_POP:
                    self.wordMC.gotoAndStop(0);
                    if(frame<25) face.gotoAndStop(frame+1);
                    break;
                case MODE_AWAY:
                    if(frame<29){
                        face.gotoAndStop(frame+1);
                    }else{

                        // Loop back
                        face.gotoAndStop(7);
                        MODE = MODE_BLINK;
                        self.isSmug = false;
                        
                    }
                    break;
            }
        }

        ///////////////////////////
        ///////////////////////////
        ///////////////////////////

        // Shocked by a square!
        if(!self.isSmug){

            if(self.gracePeriod<=0){
                var closeTo = self.touchingPeeps(90, function(peep){
                    return(!peep.offended && peep.type=="circle");
                });
                if(closeTo.length>0 && self.isWalking){

                    // Bounce back
                    self.flip = (closeTo[0].x>self.x) ? 1 : -1;
                    self.bounce = 1.5;

                    // BE SHOCKED
                    MODE = MODE_SMUG;
                    self.isWalking = false;

                    // Sound!
                    Game.sounds.peep_huh.play();

                    // They get confused!
                    closeTo.forEach(function(other){
                        if(other.offended) return;
                        other.beOffended(self);
                    });

                    // HMPH!
                    self.setTimeout(function(){
                        
                        MODE = MODE_HMPH;
                        self.isSmug = true;

                        // Sound!
                        Game.sounds.peep_hmph.play();

                        // POP!
                        self.setTimeout(function(){
                            MODE = MODE_POP;

                            // Then stop
                            self.setTimeout(function(){

                                // Turn around & walk
                                self.flip *= -1;
                                self.startWalking();
                                self.direction = (self.flip>0) ? 0 : Math.PI; // override!
                                self.direction += Math.random()*0.2-0.1;

                                // Move away...
                                MODE = MODE_AWAY;
                                self.gracePeriod = 60;

                            },_s(0.8));

                        },_s(1.4));

                    },_s(0.9));

                }
            }else{
                self.gracePeriod--;
            }

        }

    };

    // AT FIRST...
    self.watchTV = function(){

        self.clearAnims(); // just in case...
        
        // 0) Stop & look
        var tv = scene.tv;
        self.stopWalking(true);
        self.flip = (tv.x>self.x) ? 1 : -1;
        var OFFSET = 0; // (Math.abs(self.x-tv.x)-60)/100;
        var WAIT = Director.ZOOM_OUT_1_TIME + Director.SEE_VIEWERS_TIME;

        // 1) Become nervous
        self.setTimeout(function(){
            
            self.bounce = 1.6;
            MODE = MODE_STARE;

            // SQUEAK
            Game.sounds.squeak.play();

        },_s(OFFSET+BEAT*2));

        // 2) Blink...
        self.setTimeout(function(){
            self.bounce = 1.3;
            MODE = MODE_BLINK;
        },_s(OFFSET+WAIT));

        // 3) And go on.
        self.setTimeout(function(){
            // self.bounce = 1.2;
            self.startWalking();
        },_s(OFFSET+WAIT+1));

    };

}