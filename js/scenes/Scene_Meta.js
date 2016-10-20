/**********************

A SCENE PURELY FOR PROMOTIONAL REASONS

**********************/

function Scene_Meta(){

	var self = this;
	Scene.call(self);

	self.UNPAUSEABLE = true; // HACK.

	////////////
	// SET UP //
	////////////

    // Graphics!
    var g = new PIXI.Container();
    self.graphics = g;
    Game.stage.addChild(g);

	// Set Up Everything
    self.world = new World(self);
    self.tv = new TV(self);
    self.tv.y += 32;
    self.world.addProp(self.tv);

    // RECURSIVE SCREEN
	var renderTexturePoolIndex = 0;
	var renderTexturePool = [
		new PIXI.RenderTexture(Game.renderer, Game.width, Game.height),
		new PIXI.RenderTexture(Game.renderer, Game.width, Game.height)
	];
	self.stream = new PIXI.Sprite();
	self.stream.x = Game.width/2;
	self.stream.y = Game.height/2;
	self.stream.scale.x = self.stream.scale.y = 0.125;
	self.stream.anchor.x = self.stream.anchor.y = 0.5;
	g.addChild(self.stream);

	// CHYRON
	
	// Chryon container
	/*
	var chyron = new PIXI.Container();
	chyron.alpha = 1;
	g.addChild(chyron);

	// Chyron BG
	var resourceName = "chyron";
	var bg = new MakeSprite(resourceName);
	bg.scale.x = bg.scale.y = 1/2;
	chyron.addChild(bg);

	// Chyron Text
	var text = "WE BECOME WHAT WE BEHOLD";
	var fontsize=100, max=14;
	if(text.length>max){ // more than [max] chars...
		fontsize = Math.floor(max*fontsize/text.length);
	}
    var text = new PIXI.Text(text, {font:"bold "+fontsize+"px Poppins", fill:"#FFF"});
    text.scale.x = text.scale.y = 0.2;
    text.anchor.x = 0;
    text.anchor.y = 0.5;
    text.x = 45;
    text.y = 115;
    chyron.addChild(text);*/

    self.scale = 1;

    // UPDATE
    self.update = function(){

        self.world.update();

        // RECURSIVE SCREEN
        
        var matrix = new PIXI.Matrix();
	    matrix.translate(-self.graphics.x, -self.graphics.y);
	    matrix.scale(1/self.graphics.scale.x, 1/self.graphics.scale.y);
	    //matrix.translate(-sx,-sy);

		var renderTexture = renderTexturePool[renderTexturePoolIndex];
	    renderTexture.render(self.graphics, matrix);
	    renderTexturePoolIndex = (renderTexturePoolIndex+1)%renderTexturePool.length;
	    self.stream.texture = renderTexture;

	    // ZOOM OUT
	    self.scale *= 0.993;
	    if(self.scale<1){
	    	self.scale=8;
	    }
	    g.scale.x = g.scale.y = self.scale;
	    g.x = (Game.width-(Game.width*self.scale))/2;
		g.y = (Game.height-(Game.height*self.scale))/2;

    };

	// TO IMPLEMENT
	self.kill = function(){};

    self.world.clearPeeps();
    self.world.addBalancedPeeps(30);
    var peeps = self.world.peeps;
    for(var i=0;i<10;i++){
    	var randomPeep = peeps[Math.floor(Math.random()*peeps.length)];
    	randomPeep.wearingHat = true;
    	randomPeep.hatMC.gotoAndStop(15);
    }

}