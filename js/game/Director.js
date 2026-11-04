/**************************************

THE DIRECTOR: SINGLETON

When you click to take a photo, the Director...
- gets what's inside the camera's bounds
- makes the camera take the texture shot
- controls the movement of the camera & viewport
- decides what the chyron should say
- decides which TV to put it in
- decides what other events should happen around the world

**************************************/

Game.addToManifest({
	crickets: "sounds/crickets.mp3",
	breaking_news: "sounds/breaking_news.mp3"
});

function Director(scene){

	var self = this;

	// Scene
	self.scene = scene;

	// Stages
	self.callbacks = {};
	self.callback = function(callbackName){
		var cb = self.callbacks[callbackName];
		if(cb) cb(self);
	};

	// TAKE PHOTO
	self.photoData = null;
	self.photoTexture = null;
	self.isWatchingTV = false;
	self.takePhoto = function(camera){

		// Get a photo texture at Camera's position
		self.photoTexture = camera.photoTexture;

		// Which tv...
		self.tv = scene.tv;

		// Animation!
		Tween_get(self).wait(_s(0.25)) // flash
			.call(_anim.movePhoto).wait(_s(BEAT)) // tween takes 1 sec
			.call(_anim.cutToTV).wait(_s(BEAT*1.5)) // stay on TV for 1.5 secs
			.call(_anim.zoomOut1).wait(_s(BEAT*2)) // tween takes 2 secs
			.wait(_s(BEAT*1.5)) // see viewers (or lack thereof) for 1.5 sec
			.call(_anim.zoomOut2).wait(_s(BEAT*2)) // tween takes 2 secs
			.call(_anim.reset);

		// CALLBACK
		self.chyron = "[NO CHYRON]";
		self.photoData = {};
		self.photoData.audience = 0;
		self.callback("takePhoto");

	};

	// ANIM CONSTANTS
	Director.ZOOM_OUT_1_TIME = 2;
	Director.SEE_VIEWERS_TIME = 1.5 + 0.5; // hack.
	Director.ZOOM_OUT_2_TIME = 2;



	/////////////////////////////////
	///// AUDIENCE & TV HELPERS /////
	/////////////////////////////////

	self.audience_movePhoto = function(){
		var data = self.photoData;
		if(self.noSounds) return;
		if(data.audience>0 || data.audienceCircles>0 || data.audienceSquares>0){
            Game.sounds.breaking_news.play();
        }
        if(data.forceChyron){
        	Game.sounds.breaking_news.play();
        }
	};

	self.audience_cutToTV = function(doSomethingElse, filterAudience){

        // AUDIENCE?
        var data = self.photoData;
        var circlesLeft = data.audience;
        var squaresLeft = data.audience;
        if(data.HIJACK){
        	circlesLeft = data.audienceCircles;
        	squaresLeft = data.audienceSquares;
        	data.audience = data.audienceCircles+data.audienceSquares;
        }else{
        	data.audienceCircles = data.audience;
        	data.audienceSquares = data.audience;
        }
        var peeps = self.scene.world.peeps.slice(0); // clone

        // GET OUT THE WAY (AGAIN)
        for(var i=0;i<peeps.length;i++){
        	var p = peeps[i];
        	if(p._CLASS_!="NormalPeep") continue;
        	p.getOuttaTV();
        }

        // ANY AUDIENCE, AT ALL?????
        if(data.audience>0){

        	// Who to watch TV, now?
	        if(filterAudience) peeps=peeps.filter(filterAudience); // filter
	        peeps.sort(function(){ return Math.random()<0.5; }); // shuffle
	        for(var i=0;i<peeps.length;i++){

	            var p = peeps[i];
	            if(p._CLASS_!="NormalPeep") continue;

	            // Circles on the left, Squares to the right, and here I am!
	            var flip, offset;
	            var watchTV = false;
	            if(p.type=="circle" && circlesLeft>0){
	            	flip = 1;
	            	offset = 60 + (data.audienceCircles-circlesLeft)*40;
	            	watchTV = true;
	            	circlesLeft--;
	            }
	            if(p.type=="square" && squaresLeft>0){
	            	flip = -1;
	            	offset = 60 + (data.audienceSquares-squaresLeft)*40;
	            	watchTV = true;
	            	squaresLeft--;
	            }

	            // Watch the TV? Otherwise move, get out the way.
	            if(watchTV){
	                p.x = self.tv.x;
	                p.x -= flip*offset;
	                p.y = self.tv.y+Math.random(); // tiny offset to avoid glitchy depth-sort
	                if(doSomethingElse){
	                	doSomethingElse(p);
	                }else{
	                	p.watchTV();
	                }
	            }

	        }

	    }

        // Did anyone watch?
        return (data.audience>0);

	};




	////////////////////////////////
	///// CAM BOUNDS ///////////////
	////////////////////////////////

	// EVEN BETTER DECLARATIVE HELPER
	self.tryChyron = function(catchemFunc){

		// Did it catch 'em?
		var caught = catchemFunc(self);

		// "otherwise" chaining
		// if caught something, DO NO MORE. else, keep trying!
		if(caught){
			var blankChain = {
				otherwise: function(){ return blankChain; }
			};
			return blankChain;
		}else{
			return {
				otherwise: self.tryChyron
			};
		}

	};
	self.tryCut2TV = self.tryChyron; // SAME DAMN THING.

	// Declarative Helper or whatever
	// LIKE THIS:
	/*******
	var caught = d.caught({
		peeps: {_CLASS_:"NormalPeep"},
		shocked: {_CLASS_:"NormalPeep", shocked:true}
		crazy: {_CLASS_:"CrazyPeep"},
		tv: {_CLASS_:"TV"}
	});
	*******/
	self.caught = function(catchem){

		var caught = {};

		for(var selector in catchem){

			var properties = catchem[selector];
			var returnAll = properties["returnAll"];

			var caughtProps = self.getPropsInCamera(function(prop){

				// It fits all the properties...
				var isSelected = true;
				for(var key in properties){

					// Forget this one
					if(key=="returnAll") continue;

					// Test all keys
					var value = properties[key];
					if(prop[key]!=value){
						isSelected = false;
					}

				}
				return isSelected;

            });

			caught[selector] = returnAll ? caughtProps : caughtProps[0]; // return all or one?

		}

		return caught;

	};

	// Get everything that's at least 33% inside the camera frame
	self.getPropsInCamera = function(filterFunc){

		// Those caught in the camera
		var caught = [];

		// cam top-left-right-bottom
		var cam = scene.camera;
		var cl = cam.x-cam.width/2;
		var cr = cam.x+cam.width/2;
		var ct = cam.y-cam.height/2;
		var cb = cam.y+cam.height/2;

		for(var i=0;i<scene.world.props.length;i++){
			
			var prop = scene.world.props[i];

			// prop's top-left-right-bottom
			var realY = prop.y;
			if(prop.z!==undefined) realY+=prop.z;
			var pl = prop.x-prop.width/2;
			var pr = prop.x+prop.width/2;
			var pt = realY-prop.height;
			var pb = realY;
			var totalArea = prop.width*prop.height;
	
			// not overlapping at all
			if(pr<cl) continue;
			if(pl>cr) continue;
			if(pb<ct) continue;
			if(pt>cb) continue;

			// overlapping a little bit... but how much?
			var l = Math.max(cl,pl);
			var t = Math.max(ct,pt);
			var r = Math.min(cr,pr);
			var b = Math.min(cb,pb);
			var overlapArea = (r-l)*(b-t);
			var overlapRatio = overlapArea/totalArea;

			// If 33% or more, yes!
			if(overlapRatio>0.33){
				caught.push(prop);
			}

		}

		// Filter?...
		if(filterFunc){
			caught = caught.filter(filterFunc);
		}

		return caught;

	};




	////////////////////////////////
	///// ANIMATION ////////////////
	////////////////////////////////

	var _anim = {};
	_anim.movePhoto = function(){

		var cam = scene.camera;

		// Pan and scale to the current viewport. Camera owns this tween so it
		// can be retargeted if the browser resizes mid-transition.
		cam.moveToViewportCenter(_s(BEAT), Ease.circInOut);

		// CALLBACK
		self.callback("movePhoto");

	};
	_anim.cutToTV = function(){

		// YUP, WATCHING TV.
		self.isWatchingTV = true;

		// Hide Camera
		scene.camera.hide();

        // SOUND?
		var data = self.photoData;
		var fail = false;
		var nothing = data.ITS_NOTHING;
		if(!data.forceChyron){
			if(data.audience==0 && !data.audienceCircles && !data.audienceSquares){

	            Game.sounds.crickets.play();
	            fail = true;

	            // SHOW CRICKET
	            var cricket = new Cricket(scene);
		        cricket.watchTV();
		        scene.world.addProp(cricket);

		        // OR... MULTIPLE CRICKETS!
		        if(data.CAUGHT_A_CRICKET){

		        	var cricket = new Cricket(scene);
			        cricket.watchTV();
			        cricket.x += 30;
			        cricket.hopAwayTimeout += 15;
			        scene.world.addProp(cricket);

			        var cricket = new Cricket(scene);
			        cricket.watchTV();
			        cricket.x += 60;
			        cricket.hopAwayTimeout += 30;
			        scene.world.addProp(cricket);

		        }

	        }
	    }

		// Add photo texture to the TV
		var text = self.chyron;
		var tv = self.tv;
		tv.placePhoto({
			photo: self.photoTexture,
			text: text,
			fail: fail,
			nothing: nothing
		});

		// Where to cut viewport to
		self.cutViewportTo({
	    	x: tv.x + tv.offset.x,
	    	y: tv.y + tv.offset.y,
	    	scale: tv.offset.scale
	    });

	    // GET OUT THE WAY
	    var peeps = self.scene.world.peeps.slice(0);
        for(var i=0;i<peeps.length;i++){
        	var p = peeps[i];
        	if(p._CLASS_!="NormalPeep") continue;
        	p.getOuttaTV();
        }

	};
	_anim.zoomOut1 = function(){

		// Zoom out to see viewer(s)
		var tv = self.tv;
		self.tweenViewportTo(function(){
			return {
				x: (tv.x*3 + Game.width/2)/4,
				y: tv.y + tv.offset.y*3/4,
				scale: 2.5
			};
		}, _s(BEAT*2), Ease.cubicInOut);

		// CALLBACK - HACK. WHATEVER.
		self.callback("cutToTV");

		// CALLBACK
		self.callback("zoomOut1");

	};

	_anim.zoomOut2 = function(){

		// NOPE, NO LONGER WATCHING TV.
		self.isWatchingTV = false;

		// Zoom out to the whole game
		self.tweenViewportTo(function(){
			return {
				x: Game.width/2,
				y: Game.height/2,
				scale: 1
			};
		}, _s(BEAT*2), Ease.cubicInOut);

		// CALLBACK
		self.callback("zoomOut2");

	};
	_anim.reset = function(){

		// Reset Camera
		scene.camera.reset();

		// World
		self.cutViewportTo({
			x: Game.width/2,
			y: Game.height/2,
			scale: 1
		});

		// CALLBACK
		self.callback("reset");

	};

	// VIEW PORT
	self.cutViewportTo = function(view){
		var g = scene.world.graphics;
		g.pivot.x = view.x;
		g.pivot.y = view.y;
		g.scale.x = g.scale.y = view.scale;
	};
	// Retarget an in-progress viewport transition when the browser dimensions
	// change. The tween keeps its remaining time but starts from the exact
	// current camera view, avoiding jumps and stale 16:9 target coordinates.
	self._viewportTweenState = null;
	self.onViewportResize = function(previousMetrics, metrics){
		var state = self._viewportTweenState;
		if(!state) return;
		var g = scene.world.graphics;
		state.startView = {
			x: g.pivot.x,
			y: g.pivot.y,
			scale: g.scale.x
		};
		state.progressAtRebase = state.tween.t;
	};
	self.tweenViewportTo = function(view, time, easing){
		var resolveView = (typeof view === "function") ? view : function(){ return view; };
		var g = scene.world.graphics;
		var tween = {t:0};
		var state = {
			tween: tween,
			startView: { x:g.pivot.x, y:g.pivot.y, scale:g.scale.x },
			progressAtRebase: 0
		};
		self._viewportTweenState = state;

		Tween_get(tween)
			.to({t:1}, time, easing)
			.call(function(){
				if(self._viewportTweenState === state){
					self._viewportTweenState = null;
					self.update = function(){};
				}
			});

		// Interpolate the visible world rectangle, as the original Director did,
		// but resolve the destination against the current viewport every frame.
		self.update = function(){
			var target = resolveView();
			if(!target || !isFinite(target.x) || !isFinite(target.y) || !isFinite(target.scale) || target.scale <= 0){
				return;
			}

			var progress = tween.t;
			var remaining = 1 - state.progressAtRebase;
			var t = remaining <= 0 ? 1 : (progress - state.progressAtRebase) / remaining;
			t = Math.max(0, Math.min(1, t));
			t = t*t; // preserve the original geometric interpolation curve

			var start = state.startView;
			var sl = start.x - ((Game.width/2)/start.scale);
			var sr = start.x + ((Game.width/2)/start.scale);
			var st = start.y - ((Game.height/2)/start.scale);
			var sb = start.y + ((Game.height/2)/start.scale);
			var el = target.x - ((Game.width/2)/target.scale);
			var er = target.x + ((Game.width/2)/target.scale);
			var et = target.y - ((Game.height/2)/target.scale);
			var eb = target.y + ((Game.height/2)/target.scale);

			var left = sl + (el-sl)*t;
			var right = sr + (er-sr)*t;
			var top = st + (et-st)*t;
			var bottom = sb + (eb-sb)*t;
			var x = (left+right)/2;
			var y = (top+bottom)/2;
			var scale = Game.width/(right-left);

			g.scale.x = g.scale.y = scale;
			g.pivot.x = x;
			g.pivot.y = y;
		};
	};
	
	// Goof.
	self.update = function(){};

}
