Game.addToManifest({
    laptop: "sprites/laptop.png"
});

/*************

Provides a screen zooming-out feature!

**************/

function ScreenZoomOut(scene){

	var self = this;
	self.scene = scene;

	self.started = false;
	self.laptop = null;
	self.scale = 1;

	var offsetX = 816;
	var offsetY = 459;
	var baseOffset = Game.getBaseOffset();

	self.init = function(){

		if(self.started) return;
		self.started = true;

		// Put it on!
		self.laptop = MakeSprite("laptop");
		baseOffset = Game.getBaseOffset();
		self.laptop.x = -offsetX + baseOffset.x;
		self.laptop.y = -offsetY + baseOffset.y;
    	scene.graphics.addChild(self.laptop);

    	self.update();
	    
	};

	self.fixLaptop = function(){
		if(!self.started) return;
		baseOffset = Game.getBaseOffset();
        self.laptop.x = -offsetX + baseOffset.x - scene.offX/scene.scale;
        self.laptop.y = -offsetY + baseOffset.y - scene.offY/scene.scale;
	};
	self.onViewportResize = function(){
		if(!self.started) return;
		self.fixLaptop();
	};

	self.onComplete = null;
	self.completed = false;
	self.fullTimer = _s(40);
	self.timer = self.fullTimer;
	self.update = function(){

		if(self.completed) return;
		if(!self.started) return;

		//self.scale *= 0.99;
		self.scale *= 0.9996;
		scene.scale = self.scale;
		scene.x = (Game.width-(Game.width*self.scale))/2;
		scene.y = (Game.height-(Game.height*self.scale))/2;

		// IF LESS THAN 0.4, END. (HACK)
		self.timer--;
		if(self.timer<=0){
			self.completed = true;
			self.onComplete();
		}

	};

}