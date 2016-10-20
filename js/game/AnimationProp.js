/********

Just plays an animation.

*********/

function AnimationProp(scene){

	var self = this;
	self._CLASS_ = "AnimationProp";

	// Properties
	self.scene = scene;

	// Not known yet!
	self.mc = null;
	self.x = -1;
	self.y = -1;
	self.width = -1;
	self.height = -1;

	// Graphics
    var g = new PIXI.Container();
    self.graphics = g;

	// DO IT.
	self.DRAWING_SCALE = 0.65;
	self.init = function(x, y, resourceName){

		// Make it!
		var mc = MakeMovieClip(resourceName);
		mc.scale.x = mc.scale.y = self.DRAWING_SCALE;
		g.addChild(mc);

		// Position & Dimensions
		self.x = x;
		self.y = y;
		self.width = mc.width;
		self.height = mc.height;

		// MOVIECLIP
		self.mc = mc;

		// Update!
		self.update();

	};

    // Update
    // TO IMPLEMENT: YOUR OWN ANIMATION CODDE
	self.update = function(){
		g.x = self.x;
    	g.y = self.y;
    	self.updateAnimation();
	};
	self.updateAnimation = function(){};

	/////////////
	// THE END //
	/////////////

	// KILL ME
	self.kill = function(){
		var world = self.scene.world;
		world.props.splice(world.props.indexOf(self),1);
		world.layers.props.removeChild(self.graphics);
	};

}