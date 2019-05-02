/**************************************

TV:
Is a prop you can add a photo to.

**************************************/

Game.addToManifest({
	tv: "sprites/tv.png",
	chyron: "sprites/chyron.png",
	chyron2: "sprites/chyron2.png",
	chyron3: "sprites/chyron3.png"
});

function TV(scene){

	var self = this;
	self._CLASS_ = "TV";

	// Properties
	self.scene = scene;
	self.x = Game.width/2;
	self.y = Game.height/2 + 80;
	self.width = 150;
	self.height = 180;

	// Graphics
	var resources = PIXI.loader.resources;
    var g = new PIXI.Container();
    var bg = new PIXI.Sprite(resources.tv.texture);
    bg.anchor.x = 0.5;
    bg.anchor.y = 1.0;
    bg.scale.x = bg.scale.y = 0.5;
    g.addChild(bg);
    self.graphics = g;

    // Offset
    self.offset = {
    	x: 0,
    	y: -113.5,
    	scale: 8
    };

    // Photo container
    var photoContainer = new PIXI.Container();
    var conversion = 0.5; // from 1/4 to 1/8
    photoContainer.x = self.offset.x - Camera.WIDTH*0.5*conversion;
    photoContainer.y = self.offset.y - Camera.HEIGHT*0.5*conversion;
    photoContainer.scale.x = photoContainer.scale.y = conversion;
    g.addChild(photoContainer);

    // Update
	self.update = function(){
		self.updateGraphics();
	};
	self.updateGraphics = function(){
		g.x = self.x;
    	g.y = self.y;
	};

	// PHOTO
	var photo;
	self.placePhoto = function(options){

		// OPTIONS
		var photoTexture = options.photo;
		var text = options.text || "";

		// Clear screen
		photoContainer.removeChildren();

		// Add photo now
		photo = new PIXI.Sprite(photoTexture);
	    photoContainer.addChild(photo);

		// Chryon container
		var chyron = new PIXI.Container();
		chyron.alpha = 0;
		chyron.x = +15;
		Tween_get(chyron).to({alpha:1}, _s(0.5), Ease.quadInOut);
		Tween_get(chyron).to({x:0}, _s(0.8), Ease.quadInOut);
		photoContainer.addChild(chyron);

		// Chyron BG
		var resourceName;
		if(options.nothing) resourceName="chyron3";
		else if(options.fail) resourceName="chyron2";
		else resourceName="chyron";
		var bg = new MakeSprite(resourceName);
		bg.scale.x = bg.scale.y = 1/8;
		chyron.addChild(bg);

		// Chyron Text
		if(!options.nothing){
			var fontsize=50; //, max=14;
			//if(text.length>max){ // more than [max] chars...
			//	fontsize = Math.floor(max*fontsize/text.length);
			//}
		    var text = new PIXI.Text(text + "\n", {font:"bold "+fontsize+"px Cairo", align:"right", fill:"#FFF"});  // \n hack. needed when the text field cuts some of the string font's bottom
		    text.scale.x = text.scale.y = 0.2;
		    text.anchor.x = 0;
		    text.anchor.y = 0.5;
		    text.x = 30;
		    text.y = 120;
		    chyron.addChild(text);
		}

	}

	// Update!
	self.update();

}