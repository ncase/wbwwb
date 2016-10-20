Game.addToManifest({
	cursor: "sprites/misc/cursor.json"
}, true);

function Cursor(scene){

	var self = this;
	var mc = MakeMovieClip("cursor");
	mc.anchor.x = mc.anchor.y = 0;
	self.graphics = mc;
	mc.x = -100;
	mc.y = -100;

	self.click = false;
	Game.stage.mousemove = function(mouseData){
	    var pos = mouseData.data.global;
	    mc.x = pos.x+1;
	    mc.y = pos.y+1;
	};
	Game.stage.mousedown = function(mouseData){
		self.click = true;
	};
	Game.stage.mouseup = function(mouseData){
		self.click = false;
	};

	var doubles = 0;
	self.update = function(isHover){

		doubles = (doubles+1)%2; // animate on doubles!
		if(doubles!=0) return;

		if(self.click){
			mc.gotoAndStop(4);
		}else{
			if(isHover){
				if(mc.currentFrame<3) mc.gotoAndStop(mc.currentFrame+1);
			}else{
				if(mc.currentFrame>0) mc.gotoAndStop(mc.currentFrame-1);
			}
		}

	};

}