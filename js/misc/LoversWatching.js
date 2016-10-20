Game.addToManifest({
	lovers_watching: "sprites/misc/lovers_watching.json"
});
function LoversWatching(type){

	var self = this;
	var mc = MakeMovieClip("lovers_watching");
	self.graphics = mc;
	
	self.breathe = 0;
	self.breatheSpeed = 0;
	if(type=="circle"){
		mc.x = 520;
		mc.y = 380;
		self.breatheSpeed = 0.025;
		mc.gotoAndStop(0);
	}else{
		mc.x = 550;
		mc.y = 375;
		self.breatheSpeed = 0.020;
		mc.gotoAndStop(1);
	}

	self.update = function(){
		var scale = 1 + Math.sin(self.breathe)*0.02;
		mc.scale.x = scale;
		mc.scale.y = 1/scale;
		self.breathe += self.breatheSpeed;
	};

}