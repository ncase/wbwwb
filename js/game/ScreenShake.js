Game.addToManifest({
	snow: "sprites/snow.png"
});

/*************

Provides a screen-shaking feature!

**************/

function ScreenShake(scene){

	var self = this;
	self.scene = scene;

	self.snow = null;
	self.intensity = 0;
	self.shake = function(intensity){

		self.intensity = intensity;

		// Make snowscreen for the first time
		if(!self.snow){
			self.snow = MakeSprite("snow");
			self.snow.x = Game.width/2;
			self.snow.y = Game.height/2;
			self.snow.anchor.x = self.snow.anchor.y = 0.5;
			scene.graphics.addChild(self.snow);
		}else{
			self.snow.alpha = self.baseAlpha+0.35;
		}

	}

	self.baseAlpha = 0.15;
	self.update = function(){

		// Fuzz that snowscreen
		if(self.snow){
			self.snow.alpha = self.snow.alpha*0.95 + self.baseAlpha*0.05;
			self.snow.scale.x = 1 + Math.random()*0.2;
			self.snow.scale.y = 1 + Math.random()*0.2;
			if(Math.random()<0.5) self.snow.scale.x*=-1;
			if(Math.random()<0.5) self.snow.scale.y*=-1;
		}

		// Shaking intensity!
		if(self.intensity<=0.5){
			scene.offX = 0;
			scene.offY = 0;
			self.intensity = 0;
		}else{
			scene.offX = scene.scale*(Math.random()*2-1)*self.intensity;
			scene.offY = scene.scale*(Math.random()*2-1)*self.intensity;
			self.intensity*=0.95;
		}

		// Fix it...
		if(self.snow){
			self.snow.x = Game.width/2 - scene.offX;
			self.snow.y = Game.height/2 - scene.offY;
		}

	};


}