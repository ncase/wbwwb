/*****************************

ACT I: THE SETUP
1. Hat guy
2. Lovers
// then let's start escalating...

******************************/

function Stage_Start(self){

    // Create Peeps
    self.world.clearPeeps();
    self.world.addBalancedPeeps(20);

}

function Stage_Hat(self){

	// A Hat Guy
	var hat = new HatPeep(self);
    self.world.addPeep(hat);

    // Director
    self.director.callbacks = {
        takePhoto: function(d){

            // DECLARATIVE
            d.tryChyron(function(d){
                var p = d.photoData;
                var caught = d.caught({
                    hat: {_CLASS_:"HatPeep"}
                });
                if(caught.hat){
                    p.audience = 3;
                    p.caughtHat = caught.hat;
                    d.chyron = textStrings["niceHat"];
                    return true;
                }
                return false;
            }).otherwise(_chyPeeps);

        },
        movePhoto: function(d){
            d.audience_movePhoto();
        },
        cutToTV: function(d){

            // If you did indeed catch a hat peep...
            var p = d.photoData;
            if(p.caughtHat){
                self.world.addBalancedPeeps(1); // Add with moar!
                d.audience_cutToTV(function(peep){
                    peep.wearHat();
                }); // make all viewers wear HATS!
                p.caughtHat.kill(); // Get rid of hat
                Stage_Lovers(self); // Next stage
            }else{
                d.audience_cutToTV();
            }

        }
    };

}

function Stage_Lovers(self){

    // LOVERS
    var lover1 = new LoverPeep(self);
    lover1.setType("circle");
    var lover2 = new LoverPeep(self);
    lover2.setType("square");
    lover2.follow(lover1);
    self.world.addPeep(lover1);
    self.world.addPeep(lover2);

    // Director
    self.director.callbacks = {
        takePhoto: function(d){

            // MODULAR & DECLARATIVE
            d.tryChyron(_chyLovers)
             .otherwise(_chyHats)
             .otherwise(_chyPeeps);

        },
        movePhoto: function(d){
            d.audience_movePhoto();
        },
        cutToTV: function(d){

            // MODULAR & DECLARATIVE
            d.tryCut2TV(_cutLovers)
             .otherwise(_cutHats)
             .otherwise(_cutPeeps);

            // And whatever happens, just go to the next stage
            // ACT II!!!
            Stage_Screamer(self);

        }
    };

}

///////////////////////////////////////
///////////////////////////////////////
////// DECLARATIVE CHYRON MODULES /////
///////////////////////////////////////
///////////////////////////////////////

function _chyLovers(d){
    var p = d.photoData;
    var caught = d.caught({
        lover: {_CLASS_:"LoverPeep"}
    });
    if(caught.lover){
        if(caught.lover.isEmbarrassed){
            d.chyron = textStrings["outtaHere"];
        }else{
            p.caughtLovers = true;
            p.forceChyron = true;
            d.chyron = textStrings["getARoom"];
        }
        return true;
    }
    return false;
}
function _chyHats(d){
    var p = d.photoData;
    var caught = d.caught({
        hat: {_CLASS_:"NormalPeep", wearingHat:true}
    });
    if(caught.hat){
        p.audience = 1;
        p.caughtHat = true;
        d.chyron = textStrings["notCoolAnymore"];
        return true;
    }
    return false;
}
function _chyPeeps(d){
    var p = d.photoData;
    if(d.scene.camera.isOverTV(true)){
        d.chyron = textStrings["tvOnTv"];
    }else{
        var caught = d.caught({
            peeps: {_CLASS_:"NormalPeep", returnAll:true},
            crickets: {_CLASS_:"Cricket", returnAll:true}
        });
        if(caught.crickets.length>0){
            p.CAUGHT_A_CRICKET = true;
            if(caught.crickets.length==1){
                d.chyron = textStrings["cricky"];
            }else{
                d.chyron = textStrings["tooManyCrickets"];
            }
        }else if(caught.peeps.length>0){
            if(caught.peeps.length==1){
                d.chyron = textStrings["normalPeep"];
            }else{
                d.chyron = textStrings["normalPeeps"];
            }
        }else{
            p.ITS_NOTHING = true;
            d.chyron = textStrings["wowNothing"];
        }
    }
    return true;
}

///////////////////////////////////////
///////////////////////////////////////
///// DECLARATIVE CUTTING MODULES /////
///////////////////////////////////////
///////////////////////////////////////

function _cutLovers(d){
    var p = d.photoData;
    if(p.caughtLovers){
        // Crickets
        d.audience_cutToTV();
        // MAKE LOVERS EMBARRASSED
        d.scene.world.peeps.filter(function(peep){
            return peep._CLASS_=="LoverPeep";
        }).forEach(function(lover){
            lover.makeEmbarrassed();
        });
        return true;
    }else{
        return false;
    }
}
function _cutHats(d){
    var p = d.photoData;
    if(p.caughtHat){
        // Only get the hat-wearers, make 'em take off the hat.
        d.audience_cutToTV(
            function(peep){ peep.takeOffHat(); },
            function(peep){ return peep.wearingHat; }
        );
        return true;
    }else{
        // And if not, have them decrease by 1 each time anyway.
        var hatPeeps = d.scene.world.peeps.slice(0).filter(function(peep){
            return peep.wearingHat;
        });
        if(hatPeeps.length>0){
            var randomIndex = Math.floor(Math.random()*hatPeeps.length);
            hatPeeps[randomIndex].takeOffHat(true);
        }
        return false;
    }
}
function _cutPeeps(d){
    d.audience_cutToTV();
    return true;
}
