Game.addToManifest({

    face_murder: "sprites/peeps/face_murder.json",

    weapon_gun: "sprites/peeps/weapon_gun.json",
    weapon_axe: "sprites/peeps/weapon_axe.json",
    weapon_bat: "sprites/peeps/weapon_bat.json",
    weapon_shotgun: "sprites/peeps/weapon_shotgun.json",

    gunshot: "sounds/gunshot.mp3",
    shotgun: "sounds/shotgun.mp3",
    impact: "sounds/impact.mp3"

});

/****

FACE FRAMES:
00-04: crazy at screen (loop)
05-13: blink, and crazy forever (loop: 09-13)

WEAPON FRAMES:
00-05: pull out (04 is rest)
06: BAM
07-15: return to rest (04 is rest)

****/

function MurderPeep(scene){

    var self = this;
    Peep.apply(self, [scene]);
    self._CLASS_ = "MurderPeep";

    // Add the body & face sprites
    self.type = "???";
    self.bodyMC = self.addMovieClip("body");
    self.faceMC = self.addMovieClip("face_murder");
    self.weaponType = null;
    self.weaponMC = null;

    // Init with what kinda weapon?
    self.init = function(shapeType, weaponType){

        // Type
        self.type = shapeType;
        self.bodyMC.gotoAndStop((shapeType=="circle") ? 0 : 1);

        // Weapon
        self.weaponType = weaponType;
        self.weaponMC = self.addMovieClip("weapon_"+weaponType);
        self.weaponMC.gotoAndStop(0);

        // Transform
        self.x = scene.tv.x;
        self.y = scene.tv.y+Math.random(); // tiny offset to avoid glitchy depth-sort
        if(shapeType=="circle"){
            self.x -= 60;
            self.flip = 1;
            self.gracePeriod = 30;
        }else{
            self.x += 60;
            self.flip = -1;
            self.gracePeriod = 50;
        }

        // Let's Watch TV!
        self.watchTV();

    };

    // Animate on doubles
    var MODE = 0;
    var MODE_STARE = 0;
    var MODE_CRAZY = 1;
    var MODE_KILL = 2;

    // Animate!
    var doubles = 0;
    self.gracePeriod = -1;
    self.standingTime = -1;
    self.callbacks.update = function(){
        
        // Animate on doubles! ...or... TRIPLES?
        doubles = (doubles+1)%4;

        // stay within game frame
        self.stayWithinRect({
            l:100, r:860, t:100, b:480
        },0.15);

        // FRAMES: MANUALLY
        if(doubles==0){

            var frame;

            // Face
            var face = self.faceMC;
            frame = face.currentFrame;
            switch(MODE){
                case MODE_STARE:
                    if(frame<4) face.gotoAndStop(frame+1);
                    else face.gotoAndStop(0);
                    break;
                case MODE_CRAZY:
                    if(frame<13) face.gotoAndStop(frame+1);
                    else face.gotoAndStop(9);
                    break;
            }

            // Weapon
            var weapon = self.weaponMC;
            frame = weapon.currentFrame;
            if(MODE==MODE_CRAZY || MODE==MODE_KILL){
                if(frame<5){
                    weapon.gotoAndStop(frame+1);
                }
                if(frame>=6){
                    if(frame<15){
                        weapon.gotoAndStop(frame+1);
                    }else{
                        if(self.standingTime<=0){
                            self.startWalking();
                            weapon.gotoAndStop(5);
                        }else{
                            self.standingTime--;
                        }
                    }
                }
            }

        }

        ///////////////////////////
        ///////////////////////////
        ///////////////////////////

        // KILL THE OTHER KIND
        if(self.gracePeriod<=0){
            if(!self.isShocked){
                var otherType = (self.type=="circle") ? "square" : "circle";
                var closeTo = self.touchingPeeps(120, function(peep){
                    return(peep._CLASS_=="PanicPeep" && peep.type==otherType);
                });
                if(closeTo.length>0 && self.isWalking){

                    // FACE 'EM
                    self.flip = (closeTo[0].x>self.x) ? 1 : -1;

                    // USE WEAPON
                    MODE = MODE_KILL;
                    self.weaponMC.gotoAndStop(6);
                    self.gracePeriod = _s(BEAT*2);
                    self.bounce = 1.6;
                    self.standingTime = 5; // hack
                    self.stopWalking();

                    // What sound?
                    var sound = null;
                    var volume = 0.2;
                    switch(self.weaponType){
                        case "gun":
                            sound = Game.sounds.gunshot;
                            break;
                        case "axe": case "bat":
                            sound = Game.sounds.impact;
                            break;
                        case "shotgun":
                            sound = Game.sounds.shotgun;
                            break;
                    }
                    sound.volume(volume);
                    sound.play();

                    // TODO: Actually hurt 'em
                    closeTo[0].getKilledBy(self);

                }
            }
        }else if(self.isWalking){
            self.gracePeriod--;
        }

    };

    // Speed...
    self.callbacks.startWalking = function(){
        self.speed = 3;
    };

    // SAME WALK ANIM, EXCEPT: RANDOM ROTATION
    self.walkAnim = function(){
        var g = self.graphics;

        // Hop & Flip
        self.hop += self.speed/40;
        if(self.hop>1) self.hop--;
        self.flip = (self.vel.x<0) ? -1 : 1;

        // Sway back & forth
        var t = self.hop*Math.PI*2;
        g.pivot.y = Math.abs(Math.sin(t))*15;
        g.rotation = (Math.random()*2-1)*0.05;

    };

    // SAME STAND ANIM, EXCEPT: RANDOM ROTATION
    self.standAnim = function(){
        var g = self.graphics;
        g.rotation = (Math.random()*2-1)*0.05;
        g.pivot.y = 0;
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
            MODE = MODE_CRAZY;            
        },_s(OFFSET+BEAT*2));

        // 3) And go on.
        self.setTimeout(function(){
            self.startWalking();
            if(self.type=="circle"){
                self.direction = Math.PI + (Math.random()*2-1);
            }else{
                self.direction = (Math.random()*2-1);
            }
        },_s(OFFSET+WAIT+1));

    };

}