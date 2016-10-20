Game.addToManifest({
    gore: "sprites/peeps/gore.json"
});

/**************************************

Gore:
Just a prop that bounces around.

**************************************/

function Gore(scene){

	var self = this;
	self._CLASS_ = "Gore";

	// Graphics: Layers to this peep.
	self.DRAWING_SCALE = 0.65;
    var g = new PIXI.Container();
    self.graphics = g;
    self.mc = MakeMovieClip("gore");
    self.mc.gotoAndStop(Math.floor(Math.random()*3));
	self.mc.anchor.x = 0.5;
	self.mc.anchor.y = 0.5;
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
	self.z = 0;
	self.rotation = 0;
	self.gravity = 0.5;
	self.init = function(options){

		// All them dang options
		if(options.direction!==undefined) self.direction=options.direction;
		if(options.velocity!==undefined) self.velocity=options.velocity;
		if(options.x!==undefined) self.x=options.x;
		if(options.y!==undefined) self.y=options.y;
		if(options.z!==undefined) self.z=options.z;
		if(options.gravity!==undefined) self.gravity=options.gravity;

		// And then convert to vx & vz.
		self.vx = Math.cos(self.direction)*self.velocity;
		self.vz = Math.sin(self.direction)*self.velocity;
		self.vy = Math.random()-0.5; // just coz
		self.vr = (Math.random()*2-1)*0.5; // just coz

	};

	// Update!
	self.update = function(){

		// FALLING
		self.x += self.vx;
		self.y += self.vy;
		self.z += self.vz;
		self.rotation += self.vr;
		self.vz += self.gravity;

		// Bounce or no?
		if(self.z>=0){
			self.z=0;
			self.vx *= 0.8;
			self.vy *= 0.8;
			self.vr *= 0.8;
			if(Math.abs(self.vz)>1){

				// BLOOD FOR THE BLOOD GOD
				var blood = new Blood(scene);
                blood.init({
                    x: self.x,
                    y: self.y,
                    scale: Math.abs(self.vz)*0.02
                });
                scene.world.addBG(blood);

                // Bounce
                self.vz *= -0.2;

			}else{
				self.vz = 0;
			}
		}

		// Convert to Graphics!
		g.x = self.x;
		g.y = self.y + self.z;
		g.rotation = self.rotation;

	};

	// KILL ME
	self.kill = function(){
		var world = self.scene.world;
		world.props.splice(world.props.indexOf(self),1);
		world.layers.props.removeChild(self.graphics);
	};


}