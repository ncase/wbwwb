Game.addToManifest({
	blackout: "sprites/quote/quote0001.png",
	quote0002: "sprites/quote/quote0002.png",
	quote0003: "sprites/quote/quote0003.png",
	quote0004: "sprites/quote/quote0004.png",

	bg_park: "sounds/bg_park.mp3"
});

function Scene_Quote(){
	
	var self = this;
	Scene.call(self);

	// Layers, yo.
	var q1 = MakeSprite("blackout");
	var q2 = MakeSprite("quote0002");
	var q3 = MakeSprite("quote0003");
	var q4 = MakeSprite("quote0004");

	// Add 'em in.
	q2.alpha = q3.alpha = q4.alpha = 0;
	Game.stage.addChild(q1);
	var text = new PIXI.Container();
	Game.stage.addChild(text);
	text.addChild(q2);
	text.addChild(q3);
	text.addChild(q4);

	// TWEEN ANIM
	Tween_get(q2)
	.wait(_s(BEAT*1.5))
	.to({alpha:1}, _s(BEAT), Ease.quadInOut).call(function(){
		Tween_get(q3)
		.wait(_s(4.0*BEAT))
		.to({alpha:1}, _s(BEAT), Ease.quadInOut).call(function(){
			Tween_get(q4)
			.wait(_s(BEAT))
			.to({alpha:1}, _s(BEAT), Ease.quadInOut)
			.call(function(){
				
				// Background Ambience
				var ambience = Game.sounds.bg_park;
			   	ambience.loop(true);
			   	ambience.volume(0);
			   	ambience.play();
			   	ambience.fade(0, 1, 2000);

			})
			.wait(_s(BEAT*1.5))
			.call(function(){

				Tween_get(text).to({alpha:0}, _s(BEAT), Ease.quadInOut).call(function(){
					Game.sceneManager.gotoScene("Game");
				});

			});
		});
	});

}