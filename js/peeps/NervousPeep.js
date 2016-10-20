Game.addToManifest({

    face_nervous: "sprites/peeps/face_nervous.json",

    peep_gasp: "sounds/peep_gasp.mp3",
    peep_hngh: "sounds/peep_hngh.mp3"

});

/****

FRAMES:
00-03: nervous at screen
00-06: look away from screen (frame 06 is resting)
08-10: look backwards
12-16: look forward (then loop to 06)
17-22: shocked
23-25: run away
26-29: return to normal (frame 06 is resting)

****/

function NervousPeep(scene){

	var self = this;
	Peep.apply(self, [scene]);
    self._CLASS_ = "NervousPeep";

	// Add the body & face sprites
    self.bodyMC = self.addMovieClip("body");
    self.faceMC = self.addMovieClip("face_nervous");
    self.faceMC.gotoAndStop(0);

    // Animate on doubles
    var MODE = -1;
    var MODE_STARE = 0;
    var MODE_BLINK = 1;
    var MODE_SHOCKED = 2;
    var MODE_RUNAWAY = 3;
    var MODE_CALMDOWN = 4;

    // HACK
    self.HACK_JUMPSTART = function(){
        MODE = MODE_BLINK;
    };

    var doubles = 0;
    self.isShocked = false;
    self.callbacks.update = function(){
        
        // Animate on doubles! ...or... TRIPLES?
        doubles = (doubles+1)%3;

        // stay within game frame
        self.stayWithinRect({
            l:150, r:810, t:150, b:450
        },0.05);

        // FRAMES: MANUALLY
        var face = self.faceMC;
        var frame = face.currentFrame;
        if(doubles==0){
            switch(MODE){
                case MODE_STARE:
                    if(frame<3) face.gotoAndStop(frame+1);
                    break;
                case MODE_BLINK:
                    if(frame<6) face.gotoAndStop(frame+1);

                    // Look shifty...
                    if(frame==6){
                        if(Math.random()<0.05) face.gotoAndStop(frame+1);
                    }else if(frame<10){
                        face.gotoAndStop(frame+1);
                    }else if(frame==10){
                        if(Math.random()<0.05) face.gotoAndStop(frame+1);
                    }else if(frame<16){
                        face.gotoAndStop(frame+1);
                    }else if(frame==16){
                        face.gotoAndStop(6);
                    }

                    break;
                case MODE_SHOCKED:
                    if(frame<22) face.gotoAndStop(frame+1);
                    break;
                case MODE_RUNAWAY:
                    if(frame<25) face.gotoAndStop(frame+1);
                    break;
                case MODE_CALMDOWN:
                    if(frame<29){
                        face.gotoAndStop(frame+1);
                    }else{
                        face.gotoAndStop(6);
                        MODE = MODE_BLINK;
                        self.isShocked = false;
                    }
                    break;
            }
        }

        ///////////////////////////
        ///////////////////////////
        ///////////////////////////

        // Shocked by a square!
        if(!self.isShocked){
            var closeTo = self.touchingPeeps(90, function(peep){
                return(peep.isWalking && peep.type=="square");
            });
            if(closeTo.length>0 && self.isWalking){

                // BE SHOCKED
                MODE = MODE_SHOCKED;
                self.isWalking = false;
                face.gotoAndStop(17);
                self.isShocked = true;

                // Sound!
                Game.sounds.peep_gasp.play();

                // Bounce back
                self.flip = (closeTo[0].x>self.x) ? 1 : -1;
                self.bounce = 1.5;
                self.vel.x = -self.flip*5;

                // They get confused!
                closeTo.forEach(function(other){
                    if(!other.isWalking) return;
                    other.beConfused(self);
                });

                // Run away!
                self.setTimeout(function(){

                    // Sound!
                    Game.sounds.peep_hngh.volume(0.6);
                    Game.sounds.peep_hngh.play();

                    // RUN!
                    MODE = MODE_RUNAWAY;
                    self.flip *= -1;
                    self.bounce = 1.5;
                    self.startWalking();
                    self.speed = 3.5;
                    if(self.flip>0){
                        self.direction = Math.TAU*0/4;
                    }else{
                        self.direction = Math.TAU*2/4;
                    }

                    // Calm down...
                    self.setTimeout(function(){
                        MODE = MODE_CALMDOWN;
                        self.callbacks.startWalking();
                    },_s(3));


                },_s(1));

            }
        }

    };

    // Speed...
    self.callbacks.startWalking = function(){
        self.speed = 0.8;
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

    // IS SCARED?
    self.isScared = function(){
        return self.isShocked;
    };

}