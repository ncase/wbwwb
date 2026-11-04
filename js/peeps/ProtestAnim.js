Game.addToManifest({
    peace: "sprites/peeps/peace.json"
});

/****

PROTEST!!
00-30: protest
   31: SHOCK

****/

function ProtestAnim(scene){

	var self = this;
	AnimationProp.apply(self, [scene]);
    self._CLASS_ = "ProtestAnim";

	// INIT
    var position = Game.baseToWorldPosition(650, 200);
    self.init(position.x, position.y, "peace");

    // AVOID SPOT
    var spot = Game.anchorBasePosition({ radius: 150 }, 640, 150);
    scene.avoidSpots.push(spot);

    // ANIMATION CODE
    var MODE = 0;
    MODE_PROTEST = 0;
    MODE_SHOCK = 1;
    self.triples = 0;
	self.updateAnimation = function(){

    	// ANIMATE on TRIPLES
        self.triples = (self.triples+1)%3;
        if(self.triples==0){

        	var mc = self.mc;
        	var totalFrames = mc.totalFrames;
        	var frame = mc.currentFrame;

        	switch(MODE){
        		case MODE_PROTEST:
        			if(frame<30) mc.gotoAndStop(frame+1);
        			else mc.gotoAndStop(0);
        			break;
        		case MODE_SHOCK:
        			mc.gotoAndStop(31);
        			break;
        	}

        }

	};

    // FREEZE 'EM
    self.beStunned = function(){
        MODE = MODE_SHOCK;
    };

}