/**************************************

PEEP:
Walks around.

**************************************/

function Peep(scene){

	var self = this;
	self._CLASS_ = "Peep";

	// Properties
	self.scene = scene;
	self.x = Math.random()*Game.width;
	self.y = Math.random()*Game.height;
	self.DRAWING_SCALE = 0.65;
	self.width = 80*self.DRAWING_SCALE;
	self.height = 120*self.DRAWING_SCALE;

	// Callbacks
	self.callbacks = {};
	self.callback = function(callbackName){
		var cb = self.callbacks[callbackName];
		if(cb) cb(self);
	};

	// Graphics: Layers to this peep.
    var g = new PIXI.Container();
    self.graphics = g;

    // Hop! And bounce INDEPENDENT of anim. Bouncy math!
    
    self.hop = Math.random();
    self._lastHop = self.hop;

    self.speed = 1 + Math.random()*0.5;
    self.direction = Math.random()*Math.PI*2;
    self.vel = {x:0,y:0};
    self.flip = 1;
    
    self.isWalking = true;
    self.loop = true;

    self.bounce = 1;
    self.bounceVel = 0;
    self.bounceAcc = 0.2;
    self.bounceDamp = 0.6;

    self.goThroughSpots = false;
    //self.allowToStay = false;

	self.update = function(){

		/////////////
		// WALKING //
		/////////////

		// Walk velocity
		var vx, vy;
    	if(self.isWalking){
			var vx = Math.cos(self.direction)*self.speed;
			var vy = Math.sin(self.direction)*self.speed;
		}else{
			vx = vy = 0; // stand still
		}

		// Move it
		self.vel.x = self.vel.x*0.9 + vx*0.1;
		self.vel.y = self.vel.y*0.9 + vy*0.1;
		self.x += self.vel.x;
		self.y += self.vel.y;

		// Borders
		if(self.loop){
			var margin = 50;
			if(self.x<-margin) self.x=Game.width+margin;
			if(self.x>Game.width+margin) self.x=-margin;
			if(self.y<0) self.y=Game.height+margin*2;
			if(self.y>Game.height+margin*2) self.y=0;
		}

		////////////////////
		// AVOIDING SPOTS //
		////////////////////

		if(scene.avoidSpots && !self.goThroughSpots){
			var avoid = scene.avoidSpots;
			avoid.forEach(function(spot){

				var dx = spot.x-self.x;
				var dy = spot.y-self.y;
				
				if(dx*dx+dy*dy < spot.radius*spot.radius){
					self.direction = Math.atan2(dy,dx)+Math.PI; // insta-walk AWAY.
				}

			});
		}

		/*if(scene.director.isWatchingTV && !self.allowToStay){
			// TV Rect Bounds
	        var gx = Game.width/2;
	        var gy = Game.height/2;
	        var cw = (440)/2;
	        var ch = (220)/2;
	        var rect = {
	            l:gx-cw, r:gx+cw,
	            t:gy-ch, b:gy+ch+80
	        };
	        if(self.x>rect.l && self.x<rect.r && self.y>rect.t && self.y<rect.b){
				if(self.x<gx) self.x=rect.l;
				if(self.x>gx) self.x=rect.r;
				if(self.y<gy) self.y=rect.t;
				if(self.y>gy) self.y=rect.b;
			}
		}*/

		/////////////////////
		// ANIMATION QUEUE //
		/////////////////////

		if(!(self.pauseAnimsWhenNotWatching && self.scene.director.isWatchingTV)){ // HACK
			for(var i=0;i<self.animCalls.length;i++){
				var animCall = self.animCalls[i];
				animCall.ticks--;
				if(animCall.ticks<=0){
					animCall.callback(self);
					// Splice & go back one
					self.animCalls.splice(i,1);
					i--;
				}
			}
		}

		//////////////
		// GRAPHICS //
		//////////////

		// Position
		g.x = self.x;
    	g.y = self.y;

    	// Walking
    	if(self.isWalking){
    		self.walkAnim();
    	}else{
    		self.standAnim();
    	}

    	// Bounce
		self.bounceVel += (1-self.bounce)*self.bounceAcc;
		self.bounce += self.bounceVel;
		self.bounceVel *= self.bounceDamp;
		g.scale.x = self.bounce*self.flip;
		g.scale.y = 1/self.bounce;

		// Hop
		self._lastHop = self.hop;

		///////////////////
		// END: CALLBACK //
		///////////////////

		self.callback("update");

	};

	// Can be overridden
	self.walkAnim = function(){

		// Hop & Flip
		self.hop += self.speed/40;
		if(self.hop>1) self.hop--;
		self.flip = (self.vel.x<0) ? -1 : 1;

		// Sway back & forth
    	var t = self.hop*Math.PI*2;
    	g.rotation = Math.sin(t)*0.1;
    	g.pivot.y = Math.abs(Math.sin(t))*10;

    	// Squash at the bottom of your cycle
    	if(self._lastHop<0.5 && self.hop>=0.5) self.bounce = 1.2;
    	if(self._lastHop>0.9 && self.hop<=0.1) self.bounce = 1.2;

	};

	// Can be overridden
	self.standAnim = function(){
		g.rotation = 0;
    	g.pivot.y = 0;
	};


	////////////////////
	// ACTION HELPERS //
	////////////////////

	self.touchingPeeps = function(radius, filterFunc, oneSided){

		// Close to
		var touching = [];
		var peeps = scene.world.peeps;
		for(var i=0;i<peeps.length;i++){
            var other = peeps[i];
            if(other==self) continue;
            var dx = other.x-self.x;
            var dy = (other.y-self.y)*2; // height is half of width, w/e
            var dist = dx*dx+dy*dy;
            if(dist<radius*radius){

            	// One Sided?
            	if(oneSided){
            		if(self.flip>0 && other.x>self.x) touching.push(other);
            		if(self.flip<0 && other.x<self.x) touching.push(other);
            	}else{
            		touching.push(other);
            	}

            }
        }

		// Filter?...
		if(filterFunc){
			touching = touching.filter(filterFunc);
		}
        return touching;

	};

	self.stayWithinRect = function(rect, turn){
		if(self.x<rect.l && self.vel.x<0) self.direction += turn;
		if(self.x>rect.r && self.vel.x>0) self.direction += turn;
		if(self.y<rect.t && self.vel.y<0) self.direction += turn;
		if(self.y>rect.b && self.vel.y>0) self.direction += turn;
	};

	/*self.stayOutsideRect = function(rect, turn){
		if(self.x>rect.l && self.vel.x>0) self.direction += turn;
		if(self.x<rect.r && self.vel.x<0) self.direction += turn;
		if(self.y>rect.t && self.vel.y>0) self.direction += turn;
		if(self.y<rect.b && self.vel.y<0) self.direction += turn;
	};*/

	/////////////////////////////
	// SPRITE ANIMATION SHTUFF //
	/////////////////////////////

	self.animCalls = [];
	self.setTimeout = function(callback,ticks){
		self.animCalls.push({
			callback: callback,
			ticks: ticks
		});
	};
	self.clearAnims = function(){
		self.animCalls = [];
	};

	self.addMovieClip = function(resourceName){

		// Make it!
		var mc = MakeMovieClip(resourceName);
		mc.scale.x = mc.scale.y = self.DRAWING_SCALE;
		g.addChild(mc);

		// Return
		return mc;

	};



	///////////////////////////////////////////
	// THINGS THE DIRECTOR CAN TELL ME TO DO //
	///////////////////////////////////////////

	self.startWalking = function(){

		self.isWalking = true;
		self.speed = 1 + Math.random()*0.5;
    	self.direction = Math.random()*Math.PI*2;

    	// CALLBACK
		self.callback("startWalking");

	};
	self.startWalking();
	self.stopWalking = function(halt){
		if(halt){
			self.vel.x = self.vel.y = 0; // so not even sliding.
		}
		self.isWalking = false;
		self.hop = 0;
	};



	/////////////
	// THE END //
	/////////////

	// KILL ME
	self.kill = function(){
		var world = self.scene.world;
		world.peeps.splice(world.peeps.indexOf(self),1);
		world.props.splice(world.props.indexOf(self),1);
		world.layers.props.removeChild(self.graphics);
	};

	// Update!
	self.update();


}