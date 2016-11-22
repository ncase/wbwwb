/*****************************

ACT II: THE ESCALATION
1. Weird square screams at others
2. Circle becomes fearful
3. Square becomes snobby
4. Circle becomes angry...
5. Angry escalates... until...

******************************/

function Stage_Screamer(self){

    // The crazy one
    var crazy = new CrazyPeep(self);
    self.world.addPeep(crazy);

    // Director
    self.director.callbacks = {
        takePhoto: function(d){

            // IMPORTANCE:
            // 1. Screamer Dude
            // 2. Lovers
            // 3. Hats
            // 4. Shocked Peeps
            // 5. Everything else.
            d.tryChyron(function(d){
                var p = d.photoData;
                var caught = d.caught({
                    shocked: {_CLASS_:"NormalPeep", shocked:true},
                    crazy: {_CLASS_:"CrazyPeep"}
                });
                if(caught.crazy){
                    if(crazy.isScreaming()){
                        p.HIJACK = true;
                        p.audienceCircles = 1;
                        p.audienceSquares = 0;
                        p.caughtCrazy = caught.crazy;
                        d.chyron = textStrings["crazySquareAttacks"];
                    }else{
                        if(caught.shocked) d.chyron = textStrings["justMissed"];
                        else d.chyron = textStrings["somethingInteresting"];
                    }
                    return true;
                }
                return false;
            })
            .otherwise(_chyLovers)
            .otherwise(_chyHats)
            .otherwise(function(d){
                var p = d.photoData;
                var caught = d.caught({
                    shocked: {_CLASS_:"NormalPeep", shocked:true}
                });
                if(caught.shocked) d.chyron = textStrings["whoIsScreaming"];
                return caught.shocked;
            })
            .otherwise(_chyPeeps);
        
        },
        movePhoto: function(d){
            d.audience_movePhoto();
        },
        cutToTV: function(d){

            // Importance:
            // 1. Crazy
            // 2. Lovers
            // 3. Hats
            // 4. Everything Else
            d.tryCut2TV(function(d){
                var p = d.photoData;
                if(p.caughtCrazy){
                    self.world.addBalancedPeeps(1); // Add with moar!
                    d.audience_cutToTV(); // Show audience watching!
                    p.caughtCrazy.kill(); // Get rid of crazy
                    self.world.replaceWatcher("circle", new NervousPeep(self)); // Replace with nervous
                    Stage_Nervous(self); // Next stage
                    return true;
                }
                return false;
            })
            .otherwise(_cutLovers)
            .otherwise(_cutHats)
            .otherwise(_cutPeeps);
            
        }
    };

}

function Stage_Nervous(self, HACK){

    // Also, get rid of the lovers if they're still here.
    self.world.peeps.filter(function(peep){
        return peep._CLASS_=="LoverPeep";
    }).forEach(function(lover){
        lover.kill();
    });

    // HACK - The nervous one
    if(HACK){
        var nervous = new NervousPeep(self);
        self.world.addPeep(nervous);
        nervous.HACK_JUMPSTART();
    }

    // Director
    self.director.callbacks = {
        takePhoto: function(d){

            // IMPORTANCE:
            // 1. Nervous
            // 2. Hats
            // 3. Everything else.
            d.tryChyron(function(d){
                var p = d.photoData;
                var caught = d.caught({
                    confused: {_CLASS_:"NormalPeep", confused:true},
                    nervous: {_CLASS_:"NervousPeep"}
                });
                if(caught.nervous){
                    if(caught.nervous.isScared()){
                        if(caught.confused){
                            p.HIJACK = true;
                            p.audienceCircles = 0;
                            p.audienceSquares = 1;
                            p.caughtNervous = caught.nervous;
                            d.chyron = textStrings["circleFearsSquares"];
                        }else{
                            d.chyron = textStrings["whoScaresThem"];
                        }
                    }else{
                        d.chyron = textStrings["somethingInteresting"];
                        // d.chyron = textStrings["areTheyScared"];
                    }
                    return true;
                }
                return false;
            })
            .otherwise(_chyHats)
            .otherwise(_chyPeeps);
        
        },
        movePhoto: function(d){
            d.audience_movePhoto();
        },
        cutToTV: function(d){

            // IMPORTANCE:
            // 1. Nervous
            // 2. Hats
            // 3. Everything else.

            d.tryCut2TV(function(d){
                var p = d.photoData;
                if(p.caughtNervous){
                    self.world.addBalancedPeeps(1); // Add with moar!
                    d.audience_cutToTV(); // Show audience watching!
                    p.caughtNervous.kill(); // Get rid of nervous
                    self.world.replaceWatcher("square", new SnobbyPeep(self)); // Replace with snpb
                    Stage_Snobby(self); // Next stage
                    return true;
                }
                return false;
            })
            .otherwise(_cutHats)
            .otherwise(_cutPeeps);

        }
    };

}

function Stage_Snobby(self, HACK){

    // HACK - The snobby one
    if(HACK){
        var snobby = new SnobbyPeep(self);
        self.world.addPeep(snobby);
        snobby.HACK_JUMPSTART();
    }

    // Director
    self.director.callbacks = {
        takePhoto: function(d){

            // Importance:
            // 1. Snobby
            // 2. Hats
            // 3. Everything else
            
            d.tryChyron(function(d){
                var p = d.photoData;
                var caught = d.caught({
                    offended: {_CLASS_:"NormalPeep", offended:true},
                    snobby: {_CLASS_:"SnobbyPeep"}
                });
                if(caught.snobby){
                    if(caught.snobby.isSmug){
                        p.HIJACK = true;
                        p.audienceCircles = 1;
                        p.audienceSquares = 0;
                        p.caughtSnobby = caught.snobby;
                        d.chyron = textStrings["squaresSnubCircles"];
                    }else{
                        //d.chyron = textStrings["areTheySnubbed"];
                        d.chyron = textStrings["somethingInteresting"];
                    }
                    return true;
                }
                return false;
            })
            .otherwise(_chyHats)
            .otherwise(_chyPeeps);            

        },
        movePhoto: function(d){
            d.audience_movePhoto();
        },
        cutToTV: function(d){

            // Importance:
            // 1. Snobby
            // 2. Hats
            // 3. Everything else

            d.tryCut2TV(function(d){
                var p = d.photoData;
                if(p.caughtSnobby){
                    
                    // self.world.addBalancedPeeps(1); // Add with moar!

                    // Get rid of all hats
                    self.world.peeps.slice(0).filter(function(peep){
                        return peep.wearingHat;
                    }).forEach(function(hatPeep){
                        hatPeep.takeOffHat(true);
                    });

                    d.audience_cutToTV(); // Show audience watching!
                    p.caughtSnobby.kill(); // Get rid of snob

                    // Replace with angry
                    var angry = new AngryPeep(self, "circle");
                    self.world.replaceWatcher("circle",angry);

                    Stage_Angry_Escalation(self); // Next stage

                    return true;

                }
                return false;
            })
            .otherwise(_cutHats)
            .otherwise(_cutPeeps);

        }
    };

}

function Stage_Angry_Escalation(self, HACK){

    // HACK - ONE angry one
    if(HACK){
        var angry = new AngryPeep(self, "circle");
        self.world.addPeep(angry);
        angry.HACK_JUMPSTART();
    }

    // HELPING.
    var helping = new HelpingAnim(self);
    self.world.addProp(helping);

    // PROTESTERS!
    var protest = null;
    var _addProtesters = function(){

        // ONCE
        if(protest) return;
        protest = new ProtestAnim(self);

        self.world.addProp(protest);
        
    }

    // Director
    self.director.callbacks = {
        takePhoto: function(d){

            // IMPORTANCE:
            // 1. Helping
            // 2. Protest
            // 3. Angry
            // 4. Weirdo
            // 5. Shocked
            // 6. Everything Else

            d.tryChyron(_chyHelping)
             .otherwise(_chyProtest)
             .otherwise(_chyAngry)
             .otherwise(_chyWeirdo)
             .otherwise(_chyShocked)
             .otherwise(_chyPeeps)

        },
        movePhoto: function(d){
            d.audience_movePhoto();
        },
        cutToTV: function(d){

            // IMPORTANCE:
            // 1. Helping
            // 2. Protest
            // 3. Angry
            // 4. Weirdo
            // 5. Shocked
            // 6. Everything Else

            d.tryChyron(_cutHelping)
             .otherwise(_cutProtest)
             .otherwise(_cutAngry)
             .otherwise(_cutWeirdo)
             .otherwise(_cutShocked)
             .otherwise(_cutPeeps)

            // WHAT PERCENT OF PEEPS ARE NOW ANGRY???
            var peeps = d.scene.world.peeps;
            var angry = peeps.filter(function(peep){
                return(peep._CLASS_=="AngryPeep");
            });
            var angryRatio = angry.length/(peeps.length-1);
            // angryRatio = 0.95; // HACK TO SKIP

            // MORE THAN 66%: BRING IN THE PEACE PROTESTERS
            if(angryRatio>0.5){
                _addProtesters();
            }

            // ONCE (ALMOST) EVERYONE IS ANGRY, IT'S TIME FOR MURDER
            if(angryRatio==1.00){ // EXACTLY 1.
                Stage_Evil(self); // Next stage
            }

        }
    };

}


////////////////////////////////////////////////
////////////////////////////////////////////////
///////////// THE FINAL MANIFESTO //////////////
////////////////////////////////////////////////
////////////////////////////////////////////////

var _manifestoIndex = -1;
var _manifesto = textStrings["manifesto"];
function _spoutManifesto(){
    if(_manifestoIndex<_manifesto.length-1){
        _manifestoIndex++;
    }
    return _manifesto[_manifestoIndex];
}




////////////////////////////////////////////
////////////////////////////////////////////
///////////// THE FINAL THANG //////////////
////////////////////////////////////////////
////////////////////////////////////////////

function _chyAngry(d){
    var p = d.photoData;
    var caught = d.caught({
        shocked: {_CLASS_:"NormalPeep", shocked:true},
        angry: {_CLASS_:"AngryPeep", returnAll:true},
        angryCircleShouting: {_CLASS_:"AngryPeep", type:"circle", isShouting:true, returnAll:true},
        angrySquareShouting: {_CLASS_:"AngryPeep", type:"square", isShouting:true, returnAll:true},
    });
    if(caught.angry.length>0){

        if(caught.angryCircleShouting.length+caught.angrySquareShouting.length>0){

            p.audience = 2;
            p.caughtAngry = true;

            // How many angrys? (*AFTER* you add 4 more...)
            var peeps = d.scene.world.peeps;
            var angry = peeps.filter(function(peep){
                return(peep._CLASS_=="AngryPeep");
            });
            var angriesAfterwards = angry.length+4;
            var angryRatio = angriesAfterwards/(peeps.length-1);

            if(angryRatio>=1){
                d.chyron = textStrings["everyoneHates"];
            }else if(angryRatio>=0.75){
                d.chyron = textStrings["almostEveryoneHates"];
            }else{

                // Who was caught angry & shouting?
                if(caught.angryCircleShouting.length>0) p.caughtAngryCircle=true;
                if(caught.angrySquareShouting.length>0) p.caughtAngrySquare=true;

                if(caught.angryCircleShouting.length==0){
                    d.chyron = textStrings["squaresHateCircles"]; // must be a square
                }else{
                    d.chyron = textStrings["circlesHateSquares"]; // must be a circle, or both
                }

            }
            
        }else{
            if(caught.shocked){
                d.chyron = textStrings["justMissed"];
            }else{
                // d.chyron = textStrings["areTheyYelling"];
                d.chyron = textStrings["areTheyYelling"];
            }
        }
        return true;
    }
    return false;
}

function _chyHelping(d){
    var p = d.photoData;
    var caught = d.caught({
        helping: {_CLASS_:"HelpingAnim"}
    });
    if(caught.helping){
        if(caught.helping.hasHelped){
            d.chyron = _spoutManifesto();
        }else{
            d.chyron = textStrings["nerdsNow"];
        }
        return true;
    }
    return false;
}

function _chyProtest(d){
    var p = d.photoData;
    var caught = d.caught({
        protest: {_CLASS_:"ProtestAnim"}
    });
    if(caught.protest) d.chyron = _spoutManifesto();
    return caught.protest;
}

function _chyWeirdo(d){
    var p = d.photoData;
    var caught = d.caught({
        happyWeirdo: {_CLASS_:"HappyWeirdoPeep"},
        lovers: {_CLASS_:"LoverPeep"}
    });
    if(caught.happyWeirdo){
        d.chyron = _spoutManifesto();
        return true;
    }
    if(caught.lovers){
        d.chyron = _spoutManifesto();
        return true;
    }
    return false;
}

function _chyShocked(d){
    var p = d.photoData;
    var caught = d.caught({
        shocked: {_CLASS_:"NormalPeep", shocked:true}
    });
    if(caught.shocked) d.chyron = textStrings["schockedPeep"];
    return caught.shocked;
}

function _cutAngry(d){

    var p = d.photoData;
    if(p.caughtAngry){
        
        d.audience_cutToTV();

        // Make ALL the watching normal peeps angry
        var world = d.scene.world;
        var peeps = world.peeps;
        peeps.filter(function(p){
            return (p._CLASS_=="NormalPeep" && p.isWatching);
        }).forEach(function(peep){
            var newPeep = new AngryPeep(d.scene, peep.type);                 
            world.replacePeep(peep, newPeep);
            newPeep.watchTV();
        });

        return true;
    }
    return false;

}
function _cutHelping(d){
    return false;
}
function _cutProtest(d){
    return false;
}
function _cutWeirdo(d){
    return false;
}
function _cutShocked(d){
    return false;
}

////////////////////////////////////////////
////////////////////////////////////////////
////////////////////////////////////////////
////////////////////////////////////////////
////////////////////////////////////////////
////////////////////////////////////////////


function Stage_Whatever(self){

    // HACK - The snobby one
    /*
    if(HACK){
        var snobby = new SnobbyPeep(self);
        self.world.addPeep(snobby);
        snobby.HACK_JUMPSTART();
    }
    */

    // Director
    self.director.callbacks = {
        takePhoto: function(d){
            d.chyron = textStrings["whatever"];
        },
        movePhoto: function(d){
            d.audience_movePhoto();
        },
        cutToTV: function(d){
            d.audience_cutToTV();
        }
    };

}
