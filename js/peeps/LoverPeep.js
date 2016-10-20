Game.addToManifest({
    lovehat: "sprites/peeps/lovehat.json",
    lover_shirt: "sprites/peeps/lover_shirt.json"
});

/****

FRAMES for "face":
08: LUV
09: Embarrassed looking up
10: Embarrassed blink
11: Embarrassed look fwd

****/

function LoverPeep(scene){

	var self = this;
	NormalPeep.apply(self, [scene]);
    self._CLASS_ = "LoverPeep";

	// Add the body & face sprites
    self.hatMC = self.addMovieClip("lovehat");
    self.shirtMC = self.addMovieClip("lover_shirt");
    self.faceMC.gotoAndStop(8);

    // Set Type: ALSO CHANGE SHIRT
    var _oldSetType = self.setType;
    self.setType = function(type){
        _oldSetType(type);
        self.shirtMC.gotoAndStop((type=="circle") ? 0 : 1);
    };

    // STARTING POS
    self.x = 1000;
    self.y = 580;

    // Follow?
    self.follows = null;
    self.follow = function(follow){
        self.follows = follow;
        self.x = self.follows.x;
        self.y = self.follows.y;
    };
    self.callbacks.update = function(){
        if(self.follows){

            var f = self.follows;
            var tx = f.x - Math.cos(f.direction)*20;
            var ty = f.y - Math.sin(f.direction)*20;

            // LOOP THEM BOUNDS
            var margin = 50;
            var dx = tx-self.x;
            while(dx>300){
                tx -= Game.width+margin*2;
                dx = tx-self.x;
            }
            while(dx<-300){
                tx += Game.width+margin*2;
                dx = tx-self.x;
            }
            var dy = ty-self.y;
            while(dy>300){
                ty -= Game.height+margin*2;
                dy = ty-self.y;
            }
            while(dy<-300){
                ty += Game.height+margin*2;
                dy = ty-self.y;
            }

            var direction = Math.atan2(dy,dx);
            self.direction = direction;
        }else{

            // stay within game frame
            /*self.stayWithinRect({
                l:100, r:860, t:100, b:480
            },0.05);*/

        }

        if(self.follows && self.isEmbarrassed && self.x>1100){
            self.follows.kill();
            self.kill();
        }

        if(self.follows && self.x<-500){
            self.follows.kill();
            self.kill();
        }

    };
    self.callbacks.startWalking = function(){
        self.direction = 3.3; // a bit upwards from a full-left
        self.speed = 1.3;
    };
    var _pastWalkAnim = self.walkAnim;
    self.walkAnim = function(){
        if(self.follows) self.hop+=self.speed/120;
        _pastWalkAnim();
    };
    self.callbacks.startWalking();

    // STAHP. *THEN walk.*
    self.stopWalking();
    self.setTimeout(function(){
        self.startWalking();
    },_s(BEAT*4));

    // MAKE EMBARRASSED
    self.isEmbarrassed = false;
    self.makeEmbarrassed = function(){

        self.clearAnims(); // just in case...
        
        // 1) Stop & look
        var tv = scene.tv;
        self.x = tv.x;
        self.y = tv.y-5-Math.random(); // tiny offset to avoid glitchy depth-sort
        if(self.type=="square"){
            self.x += 80;
        }else{
            self.x += 120;
        }
        self.stopWalking(true);
        self.faceMC.gotoAndStop(9);
        self.flip = (tv.x>self.x) ? 1 : -1;
        var WAIT = 3.7*BEAT;
        self.isWatching = true;

        // 2) Blink...
        self.setTimeout(function(){
            
            self.isWatching = false;
            self.faceMC.gotoAndStop(10);
            self.bounce = 1.2;

        },_s(WAIT));

        // 3) And go on.
        self.setTimeout(function(){
            
            self.isEmbarrassed = true;
            self.faceMC.gotoAndStop(11);

            // GET ON OUT
            self.startWalking();
            self.loop = false;
            self.direction = 0;
            self.speed = 3;
            
        },_s(WAIT+0.06));

    };

}