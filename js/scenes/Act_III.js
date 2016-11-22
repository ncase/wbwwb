Game.addToManifest({
    bg_panic: "sounds/bg_panic.mp3",
    bg_creepy: "sounds/bg_creepy.mp3"
});

/*****************************

ACT III: THE MURDERS.
Just lots and lots of murdering.
Also, after 10 seconds, zoom out
for 20 seconds into "the real world".

******************************/

function Stage_Evil(self, HACK){

    // HACK - The happy one
    if(HACK){
        self.world.addPeep(new HappyWeirdoPeep(self));
        self.world.addProp(new ProtestAnim(self));
    }

	// FREEZE EVERYONE
	var _freezeEveryone = function(){

		var peeps = self.world.peeps;

		// Freeze all Angry & Normal peeps.
		var freezeEm = peeps.filter(function(peep){
			return(peep._CLASS_=="NormalPeep" || peep._CLASS_=="AngryPeep");
		});
		freezeEm.forEach(function(oldPeep){

			var stunnedPeep = new NormalPeep(self);
			stunnedPeep.setType(oldPeep.type);
            stunnedPeep.vel.x = oldPeep.vel.x;
            stunnedPeep.vel.y = oldPeep.vel.y;
            stunnedPeep.flip = (oldPeep.x<Game.width/2) ? 1 : -1;
            stunnedPeep.beStunned();
            self.world.replacePeep(oldPeep, stunnedPeep);
            stunnedPeep.update(); // JUST. IN. CASE.

		});

		// Freeze the Protesters
		var lovers = self.world.props.filter(function(prop){
			return prop._CLASS_=="ProtestAnim";
		})[0];
		lovers.beStunned();

        // GOODBYE PARK SOUND
        Game.sounds.bg_park.stop();

    };

    // BANG
    var _bang = function(){
        Stage_Panic(self); // Next stage
    };

    // Happy Guy is prepared to die.
    var happy = self.world.peeps.filter(function(peep){
        return peep._CLASS_=="HappyWeirdoPeep";
    })[0];
    happy.prepareForMurder();
    
    // The MURDERER
    var murderer = new EvilHatPeep(self);
    murderer.victim = happy;
    murderer.freezeEveryone = _freezeEveryone;
    murderer.bang = _bang;
    self.world.addPeep(murderer);

    // Director
    self.director.callbacks = {
        takePhoto: function(d){

            d.tryChyron(function(d){
                var p = d.photoData;
                if(murderer.hasGunOut){
                    d.chyron = textStrings["ellipsis"];
                    return true;
                }else{
                    var caught = d.caught({
                        evil: {_CLASS_:"EvilHatPeep"}
                    });
                    if(caught.evil){
                        d.chyron = textStrings["coolNoMore"];
                        return true;
                    }
                    return false;
                }
            })
            .otherwise(_chyProtest)
            .otherwise(_chyAngry)
            .otherwise(_chyWeirdo)
            .otherwise(_chyShocked)
            .otherwise(_chyPeeps);

        },
        movePhoto: function(d){
            d.audience_movePhoto();
        },
        cutToTV: function(d){
            d.audience_cutToTV();
        }
    };

}

function Stage_Panic(self){

    // CREEPY & PANIC MUSIC
    var panic = Game.sounds.bg_panic;
    panic.loop(true);
    panic.volume(0.75);
    panic.play();
    var creepyAmbience = Game.sounds.bg_creepy;
    creepyAmbience.loop(true);
    creepyAmbience.volume(0);
    creepyAmbience.play();
    creepyAmbience.fade(0, 1, 5000);

    // NO MORE SOUNDS.
    self.camera.noSounds = true;
    self.director.noSounds = true;

    // SCREEN SHAKE
    self.shaker.shake(100);

    // ONLY ONE AVOIDSPOT
    self.avoidSpots.splice(0, self.avoidSpots.length);
    self.avoidSpots.push({
        x: 530,
        y: 430,
        radius: 150
    });

    // Replace EVERYONE with a PANIC
    var peeps = self.world.peeps;
    var normalPeeps = peeps.filter(function(peep){
        return peep._CLASS_=="NormalPeep";
    });
    normalPeeps.forEach(function(oldPeep){
        var panicPeep = new PanicPeep(self);
        panicPeep.setType(oldPeep.type);
        self.world.replacePeep(oldPeep, panicPeep);
    });

    // Replace PROTEST ANIM with PANICKED LOVERS
    var lovers = self.world.props.filter(function(prop){
        return prop._CLASS_=="ProtestAnim";
    })[0];
    lovers.kill();
    var panicCircle = new PanicPeep(self);
    panicCircle.setLover("circle");
    self.world.addPeep(panicCircle);
    var panicSquare = new PanicPeep(self);
    panicSquare.setLover("square");
    self.world.addPeep(panicSquare);

    // EXPLODE THE HAPPY ONE
    var happy = self.world.peeps.filter(function(peep){
        return peep._CLASS_=="HappyWeirdoPeep";
    })[0];

    // Happy One's corpse.
    var deadbody = new DeadBody(self);
    deadbody.init({
        direction: -1.4,
        velocity: 3.5,
        x: happy.x,
        y: happy.y,
        flip: -1,
        frame: 0
    });
    self.world.addProp(deadbody);    

    // Gore Particles
    for(var i=0;i<30;i++){
        var gore = new Gore(self);
        gore.init({
            direction: -(Math.TAU/4)+(Math.random()),
            velocity: 10+Math.random()*5,
            x: happy.x,
            y: happy.y,
            z: (Math.random()*-30)
        });
        self.world.addProp(gore);
    }

    // KILL
    happy.kill();

    // Zoomer... what to do when DONE?
    self.zoomer.onComplete = function(){

        // Sounds stop
        creepyAmbience.stop();
        panic.stop();

        // Next Scene!
        Game.sceneManager.gotoScene("Credits");

    };

    // Director
    var weaponIndex = 0;
    var weapons = ["gun", "bat", "shotgun", "axe"];
    self.director.callbacks = {
        takePhoto: function(d){
            var p = d.photoData;
            d.chyron = textStrings["beScared"];
            p.forceChyron = true;
            p.noChyronSound = true;
        },
        movePhoto: function(d){
            d.audience_movePhoto();
        },
        cutToTV: function(d){
            
            d.audience_cutToTV();

            // Get rid of Hat Guy, if not done so already.
            var peeps = self.world.peeps;
            var murderer = peeps.filter(function(peep){
                return peep._CLASS_=="EvilHatPeep";
            })[0];
            if(murderer){
                murderer.kill();
                // AND NO AVOIDSPOTS ANYMORE
                self.avoidSpots.splice(0, self.avoidSpots.length);
            }

            // DELETE ANY PREV MURDERERS.
            var murderers = self.world.peeps.filter(function(peep){
                return peep._CLASS_=="MurderPeep";
            });
            murderers.forEach(function(murderer){
                murderer.kill();
            });
            if(murderers.length>0 && !self.zoomer.started){
                self.zoomer.init();
            }

            // CREATE TWO MURDERERS.
            var weapon1 = weapons[weaponIndex];
            var murderer1 = new MurderPeep(self);
            murderer1.init("circle", weapon1);
            self.world.addPeep(murderer1);
            var weapon2 = weapons[(weaponIndex+1)%weapons.length];
            var murderer2 = new MurderPeep(self);
            murderer2.init("square", weapon2);
            self.world.addPeep(murderer2);

            // Cycle through weapons...
            weaponIndex = (weaponIndex+1)%weapons.length;

        }
    };

}
