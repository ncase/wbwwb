Game.addToManifest({
    blood: "sprites/peeps/blood.json"
});

/**************************************

Just expand to proper size. Then stop.

**************************************/

function Blood(scene){

	var self = this;
	self._CLASS_ = "Blood";

	// Graphics: Layers to this peep.
	self.DRAWING_SCALE = 0.65;
    var g = new PIXI.Container();
    self.graphics = g;
    self.mc = MakeMovieClip("blood");
	self.mc.anchor.x = 0.5;
	self.mc.anchor.y = 0.5;
	self.mc.gotoAndStop(Math.floor(Math.random()*3));
	self.mc.scale.x = self.mc.scale.y = 0;
	g.addChild(self.mc);

	// What scale to go to?
	self.x = 0;
	self.y = 0;
	self.scale = 0;
	self.gotoScale = 0;
	self.init = function(options){
		self.x = options.x;
		self.y = options.y;
		self.gotoScale = options.scale;
	};

	// Update!
	self.update = function(){

		// DONE
		if(self.DONE) return;
		if(Math.abs(self.scale-self.gotoScale)<0.01) self.DONE=true;

		// Scaling...
		self.scale = (self.scale*0.8) + (self.gotoScale*0.2);

		// Convert to Graphics!
		g.x = self.x;
		g.y = self.y;
		self.mc.scale.x = self.mc.scale.y = self.scale;

	};

	// KILL HALF THE *STILL* PARTICLES WHENEVER TV CUTS OUT.
	// KILL ME
	self.kill = function(){
		var world = self.scene.world;
		world.bg.splice(world.bg.indexOf(self),1);
		world.layers.bg.removeChild(self.graphics);
	};


}