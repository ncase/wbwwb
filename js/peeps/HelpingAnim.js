Game.addToManifest({
    helping: "sprites/peeps/helping.json"
});

/****

HELPING!!
000-110: helping
(12: SCREAM!)
(70: helped!)
(and then 1 second pause...)
111: no-lover frame

****/

function HelpingAnim(scene){

	var self = this;
	AnimationProp.apply(self, [scene]);
    self._CLASS_ = "HelpingAnim";

	// INIT
    self.init(180, 170, "helping");

    // AVOID SPOT
    var spot = {
        x: self.x,
        y: self.y-40,
        radius: 170
    };
    scene.avoidSpots.push(spot);
    scene.noYellingYet = true;

    // Helped?
    self.hasHelped = false;

    // ANIMATION CODE
    var MODE = 0;
    MODE_HELPING = 0;
    self.triples = 0;
    var gracePeriod = _s(Director.ZOOM_OUT_2_TIME*0.8);
    var frame2 = 0;
	self.updateAnimation = function(){

        // ONLY PAST GRACE...
        if(scene.director.isWatchingTV){
            gracePeriod = _s(Director.ZOOM_OUT_2_TIME*0.9);
        }
        if(gracePeriod>0){
            gracePeriod--;
            return;
        }

    	// ANIMATE on TRIPLES
        // AND ONLY WHEN NOT WATCHING TV.
        self.triples = (self.triples+1)%3;
        if(self.triples==0 && !scene.director.isWatchingTV){

        	var mc = self.mc;
        	var totalFrames = mc.totalFrames;
        	var frame = mc.currentFrame;

        	switch(MODE){
        		case MODE_HELPING:
        			if(frame<110){
                        frame = frame+1;
                        mc.gotoAndStop(frame);
                        if(frame==12) Game.sounds.scream.play();
                        if(frame==70){
                            self.hasHelped=true;                            
                            Game.sounds.squeak.play();// SQUEAK
                        }
                    }else{
                        frame2++;
                        if(frame2>20){
                            mc.gotoAndStop(111);
                            self.byeLovers();
                        }
                        if(frame2>170){
                            self.byeSelf();
                        }
                    }
        			break;
        	}

        }

	};

    // REPLACE WITH WEIRDO & LOVERS
    self.loversGone = false;
    self.byeLovers = function(){

        // ONCE
        if(self.loversGone) return;
        self.loversGone = true;

        // Put in lovers #2, they just run off screen.
        var lover1 = new LoverPeep(self.scene);
        lover1.x = 188;
        lover1.y = 165;
        lover1.setType("circle");
        lover1.startWalking();
        lover1.hatMC.visible = false;
        lover1.faceMC.gotoAndStop(6);
        lover1.bounceVel = 0.22;
        lover1.loop = false;

        var lover2 = new LoverPeep(self.scene);
        lover2.follow(lover1);
        lover2.x = 235;
        lover2.y = 165;
        lover2.setType("square");
        lover2.startWalking();
        lover2.faceMC.gotoAndStop(6);
        lover2.bounceVel = 0.22;
        lover2.loop = false;

        self.scene.world.addPeep(lover1);
        self.scene.world.addPeep(lover2);

        // Remove spot
        scene.avoidSpots.splice(scene.avoidSpots.indexOf(spot),1);
        scene.noYellingYet = false;

    };
    self.byeSelf = function(){

        // Put in the Happy Weirdo!
        var happyWeirdo = new HappyWeirdoPeep(self.scene);
        self.scene.world.addPeep(happyWeirdo);

        // KILL!
        self.kill();        

    };

}