/**************************************

FEED PHONE (was: TV)
A giant phone on a stand in the park.
Every photo you take gets "posted" to it,
with a caption, a like count, and a TRENDING tag.

**************************************/

Game.addToManifest({
	tv: "sprites/tv.png"
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
	var g = new PIXI.Container();
	self.graphics = g;

	// THE PHONE ON A STAND (drawn, same hand-drawn palette as the park)
	var stand = new PIXI.Graphics();
	// pole
	stand.beginFill(0x333333);
	stand.drawRect(-3, -70, 6, 70);
	stand.endFill();
	// little base
	stand.beginFill(0x333333);
	stand.drawEllipse(0, 0, 22, 4);
	stand.endFill();
	g.addChild(stand);

	var phone = new PIXI.Graphics();
	// body
	phone.beginFill(0x2A2A2A);
	phone.drawRoundedRect(-72, -160, 144, 94, 12);
	phone.endFill();
	// screen (behind the photo)
	phone.beginFill(0x111111);
	phone.drawRect(-60, -147.25, 120, 67.5);
	phone.endFill();
	// camera dot + speaker
	phone.beginFill(0x555555);
	phone.drawCircle(-66, -113.5, 1.6);
	phone.drawRoundedRect(64, -121, 3, 15, 1.5);
	phone.endFill();
	g.addChild(phone);

	// Offset
	self.offset = {
		x: 0,
		y: -113.5,
		scale: 8
	};

	// Photo container (in camera-photo coordinates: 240 x 135)
	var photoContainer = new PIXI.Container();
	var conversion = 0.5;
	photoContainer.x = self.offset.x - Camera.WIDTH*0.5*conversion;
	photoContainer.y = self.offset.y - Camera.HEIGHT*0.5*conversion;
	photoContainer.scale.x = photoContainer.scale.y = conversion;
	g.addChild(photoContainer);

	// Idle screen
	var idle = new PIXI.Text("@peepfeed\n", {font:"bold 40px Cairo", fill:"#666666", align:"center"});
	idle.scale.x = idle.scale.y = 0.25;
	idle.anchor.x = 0.5; idle.anchor.y = 0.5;
	idle.x = Camera.WIDTH/2; idle.y = Camera.HEIGHT/2 + 4;
	photoContainer.addChild(idle);

	// Update
	self.update = function(){
		self.updateGraphics();
	};
	self.updateGraphics = function(){
		g.x = self.x;
		g.y = self.y;
	};

	// Helpers
	var W = Camera.WIDTH, H = Camera.HEIGHT; // 240 x 135
	var _formatLikes = function(n){
		if(n>=1000000) return (Math.round(n/100000)/10)+"M";
		if(n>=1000) return (Math.round(n/100)/10)+"K";
		return ""+n;
	};
	var _heart = function(x, y, r, color){
		var h = new PIXI.Graphics();
		h.beginFill(color);
		h.drawCircle(x-r*0.5, y, r*0.6);
		h.drawCircle(x+r*0.5, y, r*0.6);
		h.moveTo(x-r*1.08, y+r*0.2);
		h.lineTo(x, y+r*1.3);
		h.lineTo(x+r*1.08, y+r*0.2);
		h.lineTo(x-r*1.08, y+r*0.2);
		h.endFill();
		return h;
	};
	var _smallText = function(str, color, bold){
		var t = new PIXI.Text(str + "\n", {font:(bold?"bold ":"")+"40px Cairo", fill:color, align:"left"});
		t.scale.x = t.scale.y = 0.2;
		t.anchor.y = 0.5;
		return t;
	};

	// PHOTO -> POST
	var photo;
	self.placePhoto = function(options){

		var photoTexture = options.photo;
		var text = options.text || "";
		var likes = options.likes || 0;
		var trending = options.trending;

		photoContainer.removeChildren();

		// The photo
		photo = new PIXI.Sprite(photoTexture);
		photoContainer.addChild(photo);

		// Post UI
		var post = new PIXI.Container();
		post.alpha = 0;
		post.y = +10;
		Tween_get(post).to({alpha:1}, _s(0.5), Ease.quadInOut);
		Tween_get(post).to({y:0}, _s(0.8), Ease.quadInOut);
		photoContainer.addChild(post);

		// --- Top bar: avatar + handle (+ TRENDING tag) ---
		var top = new PIXI.Graphics();
		top.beginFill(0xFFFFFF, 0.92);
		top.drawRect(0, 0, W, 16);
		top.endFill();
		top.beginFill(0x8A6BFF);
		top.drawCircle(9, 8, 5);
		top.endFill();
		post.addChild(top);
		var handle = _smallText("@peepfeed", "#222222", true);
		handle.x = 18; handle.y = 8.5;
		post.addChild(handle);
		if(trending){
			var tag = new PIXI.Graphics();
			tag.beginFill(0xFF3355);
			tag.drawRoundedRect(W-62, 3, 58, 10, 5);
			tag.endFill();
			post.addChild(tag);
			var tagText = _smallText("TRENDING", "#FFFFFF", true);
			tagText.scale.x = tagText.scale.y = 0.16;
			tagText.anchor.x = 0.5;
			tagText.x = W-33; tagText.y = 8.5;
			post.addChild(tagText);
		}

		// --- Bottom bar: caption + likes ---
		var bottom = new PIXI.Graphics();
		bottom.beginFill(0xFFFFFF, 0.94);
		bottom.drawRect(0, H-36, W, 36);
		bottom.endFill();
		post.addChild(bottom);

		if(!options.nothing){
			var caption = new PIXI.Text(text + "\n", {font:"bold 50px Cairo", fill:"#111111", align:"left"});
			var s = 0.2;
			if(caption.width*s > W-12) s = (W-12)/caption.width;
			caption.scale.x = caption.scale.y = s;
			caption.anchor.y = 0.5;
			caption.x = 6;
			caption.y = H-25;
			post.addChild(caption);
		}else{
			var nothing = _smallText(textStrings["chyronNothing"], "#111111", true);
			nothing.x = 6; nothing.y = H-25;
			post.addChild(nothing);
		}

		var heartColor = (likes>0) ? 0xFF3355 : 0xBBBBBB;
		post.addChild(_heart(11, H-10, 4, heartColor));
		var likeStr = _formatLikes(likes) + " " + textStrings["likes"];
		if(likes>0) likeStr += "   " + _formatLikes(Math.floor(likes*0.37)) + " " + textStrings["shares"];
		var likeText = _smallText(likeStr, (likes>0) ? "#FF3355" : "#999999", true);
		likeText.x = 19; likeText.y = H-8;
		post.addChild(likeText);

		// Count the likes up, for that dopamine hit
		if(likes>0){
			var counter = {n:0};
			Tween_get(counter).to({n:likes}, _s(1.2), Ease.quadOut);
			var _tick = function(){
				var n = Math.floor(counter.n);
				var str = _formatLikes(n) + " " + textStrings["likes"] + "   " + _formatLikes(Math.floor(n*0.37)) + " " + textStrings["shares"];
				likeText.text = str + "\n";
				if(counter.n<likes) requestAnimationFrame(_tick);
			};
			requestAnimationFrame(_tick);
		}

	};

	// Update!
	self.update();

}
