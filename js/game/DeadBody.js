Game.addToManifest({
    gore_bodies: "sprites/peeps/gore_bodies.json"
});

/**************************************

Bodies:
Just a prop that bounces around.

**************************************/

function DeadBody(scene){

	var self = this;
	self._CLASS_ = "DeadBody";

	// Graphics: Layers to this peep.
	self.DRAWING_SCALE = 0.65;
    var g = new PIXI.Container();
    self.graphics = g;
    self.mc = MakeMovieClip("gore_bodies");
	self.mc.anchor.x = 132/160;
	self.mc.anchor.y = 91/120;
	self.mc.scale.x = self.mc.scale.y = self.DRAWING_SCALE;
	g.addChild(self.mc);

	// Init – set DIRECTION and VELOCITY and X,Y,Z
	// and GRAVITY and SPIN and BOUNCE and OFFSET and FRAME
	self.direction = 0;
	self.velocity = 0;
	self.vx = 0;
	self.vz = 0;
	self.x = 0;
	self.y = 0;
	self.z = -10;
	self.gravity = 0.2;
	self.flip = 1;
	self.init = function(options){

		// All them dang options
		if(options.direction!==undefined) self.direction=options.direction;
		if(options.velocity!==undefined) self.velocity=options.velocity;
		if(options.x!==undefined) self.x=options.x;
		if(options.y!==undefined) self.y=options.y;
		if(options.z!==undefined) self.z=options.z;
		if(options.gravity!==undefined) self.gravity=options.gravity;
		if(options.flip!==undefined) self.flip=options.flip;

		// Frame
		if(options.frame!==undefined) self.mc.gotoAndStop(options.frame);

		// And then convert to vx & vz.
		self.vx = Math.cos(self.direction)*self.velocity;
		self.vz = Math.sin(self.direction)*self.velocity;

		// BLOOD FOR THE BLOOD GOD
	    var blood = new Blood(scene);
	    blood.init({
	        x: self.x,
	        y: self.y,
	        scale: 1
	    });
	    scene.world.addBG(blood);

	};

	// Update!
	self.update = function(){

		// FALLING
		self.x += self.vx;
		self.z += self.vz;
		self.vz += self.gravity;

		// Bounce or no?
		if(self.z>=0){
			self.z=0;
			self.vx *= 0.8;
			self.vr *= 0.8;
			if(Math.abs(self.vz)>1){

				// BLOOD FOR THE BLOOD GOD
				var blood = new Blood(scene);
                blood.init({
                    x: self.x,
                    y: self.y,
                    scale: 0.5 + Math.abs(self.vz)*0.1
                });
                scene.world.addBG(blood);

                // Bounce
                self.vz *= -0.3;

			}else{
				self.vz = 0;
			}
		}

		// Rotation: HOW MUCH OFF THE GROUND?
		// 0 -> 0, -50 -> TAU/4
		self.rotation = (Math.abs(self.z)/50)*(Math.TAU/4);
		if(self.rotation>Math.TAU*0.2){
			self.rotation = Math.TAU*0.2;
		}

		// Convert to Graphics!
		g.x = self.x;
		g.y = self.y + self.z;
		self.mc.rotation = self.rotation;
		g.scale.x = (self.flip>0) ? 1 : -1;

	};

	// KILL ME
	self.kill = function(){
		var world = self.scene.world;
		world.props.splice(world.props.indexOf(self),1);
		world.layers.props.removeChild(self.graphics);
	};


}