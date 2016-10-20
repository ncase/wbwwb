Game.addToManifest({
	body: "sprites/peeps/body.json",
    face: "sprites/peeps/face.json",
    hat: "sprites/peeps/hat.json",

    squeak: "sounds/squeak.mp3"
});

/****

FRAMES for "body":
0: circle
1: square

FRAMES for "face":
0: normal
1: looking at TV
2: blink
3: spooked
4: confused
5: offended
6: ^_^
7: ashamed

****/

function NormalPeep(scene){

	var self = this;
	Peep.apply(self, [scene]);
    self._CLASS_ = "NormalPeep";

	// Add the body & face sprites
    self.hatMC = self.addMovieClip("hat");
    self.bodyMC = self.addMovieClip("body");
    self.faceMC = self.addMovieClip("face");

    // Set Type: Am I a circle or square?
    self.type = "???";
    self.setType = function(type){
        self.type = type;
        self.bodyMC.gotoAndStop((type=="circle") ? 0 : 1);
    };

    // Animation
    var doubles = 0;
    self.callbacks.update = function(){
        
        // Animate on doubles! ...or... TRIPLES?
        doubles = (doubles+1)%3;

        // FRAMES: MANUALLY ANIMATE HAT
        var hat = self.hatMC;
        var frame = hat.currentFrame;
        if(doubles==0){
            if(self.wearingHat && frame<15){
                frame++;
                hat.gotoAndStop(frame);
                if(frame==11){
                    self.bounce=1.6;
                    Game.sounds.squeak.play();
                }
                if(frame==15) self.faceMC.gotoAndStop(6);
            }
            if(!self.wearingHat && frame>=15){
                frame++;
                hat.gotoAndStop(frame);
                if(frame==26) hat.gotoAndStop(0);
            }
        }

    }


    ///////////////////////////////////
    // STUNNED ////////////////////////

    self.stunned = false;
    self.beStunned = function(){
        self.stunned = true;
        self.stopWalking();
        self.faceMC.gotoAndStop(3);
    }

    var _oldStartWalking = self.startWalking;
    self.startWalking = function(){
        if(self.stunned) return;
        _oldStartWalking();
    }


    ///////////////////////////////////
    // SHOCKED ////////////////////////

    self.shocked = false;
    self.beShocked = function(){
        
        self.shocked = true;
        self.stopWalking();
        self.bounce = 2;
        self.faceMC.gotoAndStop(3);

        self.setTimeout(function(){
            self.faceMC.gotoAndStop(0);
            self.startWalking();
            self.shocked = false;
        },_s(2));

    }

    ///////////////////////////////////
    // CONFUSED ///////////////////////

    self.confused = false;
    self.beConfused = function(target){

        self.flip = (target.x>self.x) ? 1 : -1;
        
        self.confused = true;
        self.stopWalking();
        self.bounce = 1.1;
        self.faceMC.gotoAndStop(2);

        self.setTimeout(function(){
            self.faceMC.gotoAndStop(4);
        },_s(0.2));

        self.setTimeout(function(){

            self.faceMC.gotoAndStop(2);
            self.setTimeout(function(){
                self.faceMC.gotoAndStop(0);
            },_s(0.2));

            self.startWalking();
            self.confused = false;

        },_s(2.2));

    };

    ///////////////////////////////////
    // OFFENDED ///////////////////////

    self.offended = false;
    self.beOffended = function(target){

        self.flip = (target.x>self.x) ? 1 : -1;
        self.offended = true;
        self.stopWalking();

        self.clearAnims(); // just in case...

        // Blink
        self.bounce = 1.2;
        self.faceMC.gotoAndStop(2);
        self.setTimeout(function(){
            self.faceMC.gotoAndStop(0);
        },_s(0.15));

        // Blink - Pissed.
        self.setTimeout(function(){

            self.bounce = 1.2;
            self.faceMC.gotoAndStop(2);
            self.setTimeout(function(){
                self.faceMC.gotoAndStop(5); // PISSED.
            },_s(0.15));

        },_s(1.2));

        // Walk away
        self.setTimeout(function(){
            self.startWalking();
        },_s(3));

        // Stop being pissed
        self.setTimeout(function(){
            self.faceMC.gotoAndStop(2);
            self.setTimeout(function(){
                self.faceMC.gotoAndStop(0);
                self.offended = false;
            },_s(0.2));
        },_s(8));

    };

    ////////////////////////////////////
    // WEAR A HAT *WHILE WATCHING TV* //

    self.wearingHat = false;
    self.wearHat = function(){

        self.clearAnims(); // just in case...
        
        // 1) Stop & look
        var tv = scene.tv;
        self.stopWalking(true);
        self.faceMC.gotoAndStop(1);
        self.flip = (tv.x>self.x) ? 1 : -1;
        var WAIT = Director.ZOOM_OUT_1_TIME + Director.SEE_VIEWERS_TIME;
        WAIT += Math.random()*0.4; // random offset
        self.isWatching = true;

        // 2) Wear HAT! IN SYNCHRONIZED TIME
        var HAT_TIME = Director.ZOOM_OUT_1_TIME + (Math.abs(self.x-tv.x)-60)/100;
        self.setTimeout(function(){
            self.wearingHat = true;
        },_s(HAT_TIME));

        // 3) And go on.
        self.setTimeout(function(){
            self.isWatching = false;
            self.bounce = 1.2;
            self.startWalking();
        },_s(WAIT+1));

    };
    self.takeOffHat = function(instant){

        self.clearAnims(); // just in case...

        if(!instant){
        
            // 1) Stop & look
            var tv = scene.tv;
            self.stopWalking(true);
            self.faceMC.gotoAndStop(1);
            self.flip = (tv.x>self.x) ? 1 : -1;
            var WAIT = 4*BEAT + Math.random()*0.4;
            self.isWatching = true;

            // 2) Take off HAT!
            self.setTimeout(function(){
                self.wearingHat = false;
                self.bounce = 1.1;

                // Blink, then shame.
                self.faceMC.gotoAndStop(2);
                self.setTimeout(function(){
                    self.faceMC.gotoAndStop(7);
                },_s(0.2));

            },_s( BEAT*1.75 + Math.random()*0.75 ));

            // 3) And go on.
            self.setTimeout(function(){
                self.isWatching = false;
                self.bounce = 1.2;
                self.startWalking();
                self.faceMC.gotoAndStop(0);
            },_s(WAIT+0.06));

        }else{
            self.faceMC.gotoAndStop(0);
            self.hatMC.gotoAndStop(0);
            self.wearingHat = false;
        }

    };

    ///////////////////////////////////////////
    // THINGS THE DIRECTOR CAN TELL ME TO DO //
    ///////////////////////////////////////////

    self.getOuttaTV = function(){

        // just in case...
        self.shocked = false;
        self.confused = false;
        
        // TV Rect Bounds
        var gx = Game.width/2;
        var gy = Game.height/2;
        var cw = (440)/2;
        var ch = (220)/2;
        var bounds = {
            l:gx-cw, r:gx+cw,
            t:gy-ch, b:gy+ch+80
        };

        // While within those bounds, go literally anywhere else
        while(self.x>bounds.l && self.x<bounds.r
           && self.y>bounds.t && self.y<bounds.b){
            self.x = Math.random()*Game.width;
            self.y = Math.random()*Game.height;
        }

        // Stop walking until cam zooms out.
        var WAIT = Director.ZOOM_OUT_1_TIME + Director.SEE_VIEWERS_TIME;
        self.stopWalking(true);
        self.setTimeout(function(){
            self.startWalking();
        },_s(WAIT));

    };

    // WATCH TV
    self.isWatching = false;
    self.watchTV = function(){

        self.clearAnims(); // just in case...
        
        // 1) Stop & look
        var tv = scene.tv;
        self.stopWalking(true);
        self.faceMC.gotoAndStop(1);
        self.flip = (tv.x>self.x) ? 1 : -1;
        var WAIT = Director.ZOOM_OUT_1_TIME + Director.SEE_VIEWERS_TIME;
        WAIT += Math.random()*0.4; // random offset
        self.isWatching = true;

        // 2) Blink...
        self.setTimeout(function(){
            self.isWatching = false;
            self.faceMC.gotoAndStop(2);
            self.bounce = 1.2;
        },_s(WAIT));

        // 3) And go on.
        self.setTimeout(function(){
            self.faceMC.gotoAndStop(0);
            self.startWalking();
        },_s(WAIT+0.06));

    };

}