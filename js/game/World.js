Game.addToManifest({
	bg: "sprites/bg.png",
	bg_dark: "sprites/bg_dark.png"
});

/**************************************

WORLD:
Layers for the background, the people, objects, particles, etc

**************************************/

function World(scene, options){

	var self = this;
	options = options || {};

	// Properties
	self.scene = scene;

	// PIXI GRAPHICS
	var g = new PIXI.Container();
	self.graphics = g;
	g.x = g.pivot.x = Game.width/2;
	g.y = g.pivot.y = Game.height/2;
	scene.graphics.addChild(self.graphics);
	self.layers = {};

	// LAYER: BG
	self.layers.bg = new PIXI.Container();
	g.addChild(self.layers.bg);
	var bg = MakeSprite(options.bg ? options.bg : "bg");
	bg.position.x = -100;
    bg.position.y = -100;
    self.layers.bg.addChild(bg);
    self.bg = [];
    self.addBG = function(bg){
		self.bg.push(bg);
		self.layers.bg.addChild(bg.graphics);
    };

	// LAYER: PROPS
	self.layers.props = new PIXI.Container();
	g.addChild(self.layers.props);
	self.props = [];
	self.addProp = function(prop){
		prop.graphics._REFERENCE_ = prop; // HACK. REFERENCE.
		self.props.push(prop);
		self.layers.props.addChild(prop.graphics);
	};

	// SUB-LAYER: PEEPS
	self.peeps = [];
	self.addPeep = function(peep){
		self.peeps.push(peep);
		self.addProp(peep);
	};
	self.clearPeeps = function(){
		while(self.peeps.length>0){
			self.peeps[0].kill();
		}
	};
	self.addBalancedPeeps = function(howMany){

		// Count circles & squares...
		var circleCount = 0;
		var squareCount = 0;
		for(var i=0;i<self.peeps.length;i++){
			var p = self.peeps[i];
			if(p._CLASS_!="NormalPeep") continue;
			if(p.type=="circle") circleCount++;
			if(p.type=="square") squareCount++;
		}

		// Add how many?
		howMany = howMany || 1;
		for(var i=0;i<howMany;i++){
			var peep = new NormalPeep(self.scene);
			// ADD TO BALANCE 'EM!
			if(circleCount<squareCount){
	        	peep.setType("circle");
	        	circleCount++;
	        }else{
	        	peep.setType("square");
	        	squareCount++;
	        }
	        self.addPeep(peep);
	    }


	};
	self.replacePeep = function(oldPeep, newPeep){
		newPeep.x = oldPeep.x;
		newPeep.y = oldPeep.y;
		if(self.peeps.indexOf(newPeep)<0){
			self.addPeep(newPeep);
		}
		oldPeep.kill();
		return newPeep;
	};
	self.replaceWatcher = function(type, newPeep){

		// Get a RANDOM peep!
		var watchers = self.peeps.filter(function(peep){
			return (peep.type==type && peep.isWatching);
		});
		var randomIndex = Math.floor(Math.random()*watchers.length);
		var watcher = watchers[randomIndex];

		// REPLACE THIS ONE
		self.replacePeep(watcher, newPeep);
		newPeep.watchTV();

	};

    // Update
    self.update = function(){

    	// BGs: Update all
    	for(var i=0; i<self.bg.length; i++){
    		self.bg[i].update();
    	}

		// PROPS: Update then depth-sort
    	for(var i=0; i<self.props.length; i++){
    		self.props[i].update();
    	}
	    self.layers.props.children.sort(function(a,b){
	    	var ay = a._REFERENCE_.y || a.y;
	    	var by = b._REFERENCE_.y || b.y;
			return (ay<by) ? -1 : 1;
		});

	}

};

