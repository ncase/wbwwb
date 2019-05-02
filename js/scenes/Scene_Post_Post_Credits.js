Game.addToManifest({
	
	logo: "sprites/postcredits/logo.png",
	
	facebook: "sprites/postcredits/facebook.png",
	twitter: "sprites/postcredits/twitter.png",

	end_button: "sprites/postcredits/end_button.json"

});

function Scene_Post_Post_Credits(){
	
	var self = this;
	Scene.call(self);

	self.UNPAUSEABLE = true; // HACK.

	// Layers, yo.
	Game.stage.addChild(MakeSprite("blackout"));
	var cont = new PIXI.Container();
	Game.stage.addChild(cont);
	cont.visible = false;
	cont.addChild(MakeSprite("logo"));

	// _addButton
	var isHovering = false;
	var _addButton = function(x, labelFrame, callback){

		var button = new PIXI.Container();
		button.x = x;
		button.y = 325;
		cont.addChild(button);

		var bg = MakeMovieClip("end_button");
		bg.anchor.x = bg.anchor.y = 0.5;
		button.addChild(bg);

		var label = MakeMovieClip("end_button");
		label.anchor.x = label.anchor.y = 0.5;
		label.gotoAndStop(labelFrame);
		button.addChild(label);

		// INTERACTIVITY!
		button.interactive = true;
		button.mouseover = button.touchstart = function(){
			isHovering = true;
			bg.gotoAndStop(1);
			Tween_get(button.scale).to({x:1.05, y:1.05}, _s(0.2));
		};
		button.mouseout = function(){
			isHovering = false;
			bg.gotoAndStop(0);
			Tween_get(button.scale).to({x:1, y:1}, _s(0.2));
		};
		button.mousedown = button.touchend = function(){
			isHovering = false;
			Game.sounds.squeak.play();
			callback();
		};

	};
	_addButton(250, 2, function(){
		window.open("http://afzl95.github.io/");
	});
	_addButton(480, 3, function(){
		window.open("https://twitter.com/ali_fzl95");
	});
	_addButton(710, 4, function(){
		Game.sceneManager.gotoScene("Quote");
	});

	// _addSocialButton
	var _addSocialButton = function(x, icon, callback){

		var button = MakeSprite(icon);
		button.x = x;
		button.y = 419;
		button.anchor.x = button.anchor.y = 0.5;
		cont.addChild(button);

		// INTERACTIVITY!
		button.interactive = true;
		button.mouseover = button.touchstart = function(){
			isHovering = true;
			Tween_get(button.scale).to({x:1.2, y:1.2}, _s(0.2));
		};
		button.mouseout = function(){
			isHovering = false;
			Tween_get(button.scale).to({x:1, y:1}, _s(0.2));
		};
		button.mousedown = button.touchend = function(){
			isHovering = false;
			Game.sounds.squeak.play();
			callback();
		};

	};
	var text = encodeURIComponent(window.SHARE_TEXT);
	var url = encodeURIComponent(window.SHARE_URL);
	_addSocialButton(Game.width/2 - 38, "facebook", function(){
		var href = "https://www.facebook.com/sharer/sharer.php?u="+url+"&t="+text;
		window.open(href);
	});
	_addSocialButton(Game.width/2 + 13, "twitter", function(){
		var href = "https://twitter.com/intent/tweet?text="+text+"%0a"+url;
		//var href = "https://twitter.com/intent/tweet?text="+text+"%20"+url+"&via=ncasenmare";
		window.open(href);
	});

	// CURSOR
    var cursor = new Cursor(self);
    var g = cursor.graphics;
    cont.addChild(g);
    g.scale.x = g.scale.y = 0.5;
    g.x = Game.width/2;
    g.y = Game.height/2;

	// TWEEN ANIM
	Tween_get(cont)
	.wait(_s(BEAT*2))
	.call(function(){
		cont.visible = true;
	});

	// Update!
	self.update = function(){
		cursor.update(isHovering);
	}

}
