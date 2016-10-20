Game.addToManifest({
    lover_panic: "sprites/peeps/lover_panic.json"
});

/****

I guess they're just NormalPeeps with screaming face & running REAL fast

****/

function PanicPeep(scene){

	var self = this;
    NormalPeep.apply(self, [scene]);
    self._CLASS_ = "PanicPeep";

	// Add the body & face sprites
    self.faceMC.gotoAndStop(12);

    self.callbacks.startWalking = function(){
        self.speed = 3+Math.random()*2;
    };
    self.startWalking();

    // Is Lover?
    self.setLover = function(type){

        if(type=="circle"){
            self.x = 635;
            self.y = 200;
            self.direction = Math.TAU*-0.12;
        }else{
            self.x = 665;
            self.y = 200;
            self.direction = Math.TAU*-0.10;
        }

        self.loverMC = self.addMovieClip("lover_panic");
        self.loverMC.gotoAndStop( (type=="circle") ? 0 : 1 );
        self.loop = false;
        self.isLover = true;

    };
    self.callbacks.update = function(){
        if(self.isLover){
            if(self.y<-500){
                self.kill();
            }
        }
    };

    // Can be overridden
    self.walkAnim = function(){

        var g = self.graphics;

        // Hop & Flip
        self.hop += self.speed/60;
        if(self.hop>1) self.hop--;
        self.flip = (self.vel.x<0) ? -1 : 1;

        // Sway back & forth
        var t = self.hop*Math.PI*2;
        g.rotation = Math.sin(t)*0.3;
        g.pivot.y = Math.abs(Math.sin(t))*15;

        // Squash at the bottom of your cycle
        if(self._lastHop<0.5 && self.hop>=0.5) self.bounce = 1.2;
        if(self._lastHop>0.9 && self.hop<=0.1) self.bounce = 1.2;

    };

    // GET KILLED BY
    self.getKilledBy = function(killer){

        var CORPSE_FRAME, CORPSE_VELOCITY, GORE_AMOUNT;
        switch(killer.weaponType){
            case "gun":
                CORPSE_FRAME = 0;
                CORPSE_VELOCITY = 2;
                GORE_AMOUNT = 5;
                break;
            case "bat":
                CORPSE_FRAME = 1;
                CORPSE_VELOCITY = 5;
                GORE_AMOUNT = 15;
                break;
            case "shotgun":
                CORPSE_FRAME = 2;
                CORPSE_VELOCITY = 10;
                GORE_AMOUNT = 30;
                break;
            case "axe":
                CORPSE_FRAME = 3;
                CORPSE_VELOCITY = 5;
                GORE_AMOUNT = 15;
                break;
        }

        // SCREEN SHAKE
        scene.shaker.shake(30);

        // MY CORPSE
        var flip = (killer.x<self.x) ? -1 : 1;
        var frameOffset = (self.type=="circle") ? 0 : 1;
        var deadbody = new DeadBody(scene);
        deadbody.init({
            direction: -Math.TAU/4 - flip*0.7,
            velocity: CORPSE_VELOCITY,
            x: self.x,
            y: self.y,
            flip: flip,
            frame: (CORPSE_FRAME+2)*2 + frameOffset
        });
        scene.world.addProp(deadbody);    

        // MY GORE
        for(var i=0;i<GORE_AMOUNT;i++){
            var gore = new Gore(scene);
            gore.init({
                direction: -Math.TAU/4 - flip*Math.random()*0.5,
                velocity: CORPSE_VELOCITY+Math.random()*7,
                x: self.x,
                y: self.y,
                z: (Math.random()*-30)
            });
            scene.world.addProp(gore);
        }

        // KILL
        self.kill();

        // Create a new one!
        var panicPeep = new PanicPeep(scene);
        panicPeep.setType(self.type);
        panicPeep.x = (Math.random()<0.5) ? -50 : Game.width+50;
        panicPeep.y = Game.height*Math.random();
        scene.world.addPeep(panicPeep);

    };

}