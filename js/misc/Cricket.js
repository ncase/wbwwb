Game.addToManifest({
	cricket: "sprites/misc/cricket.json"
});
function Cricket(scene){

	var self = this;
	self.scene = scene;

	self._CLASS_ = "Cricket";

	var mc = MakeMovieClip("cricket");
	self.graphics = mc;
	self.mc = mc;
	var DRAWING_SCALE = 0.25;
	mc.scale.x = mc.scale.y = DRAWING_SCALE;

	self.width = 137*DRAWING_SCALE;
	self.height = 137*DRAWING_SCALE;

	var MODE = 0;
	var MODE_CHIRP = 0;
	var MODE_HOP = 1;
	
	self.flip = 1;
	self.period = 10;
	self.breathe = Math.floor(Math.random()*self.period);
	self.hop = 0;

	self.x = self.y = self.z = 0;

	self.update = function(){

		if(MODE==MODE_CHIRP){
			self.breathe++;
			if(self.breathe>self.period+10) self.breathe=0;
			if(self.breathe>self.period){
				var scale;
				if(self.breathe%4==0) scale=1.1;
				if(self.breathe%4==1) scale=1.0;
				if(self.breathe%4==2) scale=0.9;
				if(self.breathe%4==3) scale=1.0;
				mc.scale.x = DRAWING_SCALE*(scale);
				mc.scale.y = DRAWING_SCALE*(1/scale);
			}else{
				mc.scale.x = mc.scale.y = DRAWING_SCALE;
			}
		}
		if(self.hopAwayTimeout>0){
			self.hopAwayTimeout--;
			if(self.hopAwayTimeout==0) MODE=MODE_HOP;
		}
		if(MODE==MODE_HOP){
			var tv = scene.tv;
			self.flip = 1;
			self.x += 3.5;
			self.hop += 0.1570795;
			self.z = -Math.abs(Math.sin(self.hop))*100;
			self.y = tv.y;
		}
		if(self.x>Game.width+50){
			self.kill();
		}

		mc.scale.x = self.flip*Math.abs(mc.scale.x);
		mc.x = self.x;
		mc.y = self.y+self.z;

	};

	//////////////
	// WATCH TV //
	//////////////

	self.hopAwayTimeout = -1;
	self.watchTV = function(){
        
        // 1) Stop & look
		var tv = scene.tv;
		self.x = tv.x + 100;
		self.y = tv.y;
		self.flip = -1;
        var WAIT = Director.ZOOM_OUT_1_TIME + Director.SEE_VIEWERS_TIME + 2.3;

        // 2) And go on.
		self.hopAwayTimeout = _s(WAIT);

    };

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