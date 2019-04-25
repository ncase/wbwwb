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
	var q2 = new PIXI.Container();
	var q3 = new PIXI.Container();
	var q4 = new PIXI.Container();
    
    // Text
    var q2Sprite = MakeSprite("quote0002");
    q2.addChild(q2Sprite);
    
    var q2TitleText = new PIXI.Text(textStrings["WBWWB"] + "\n", {font:"65px Cairo", fill:"#FFFFFF", align:"center"});
    q2TitleText.anchor.x = 0.5;
    q2TitleText.anchor.y = 0.5;
    q2TitleText.x = Game.width / 2 + 6;
    q2TitleText.y = Game.height / 2 - 40;
    q2.addChild(q2TitleText);
    
    // dynamic fontsize for quote
    var quoteString = textStrings["WSOTATOTSU"]
    var fontsize=39, max=47;
    if(quoteString.length>max){ // more than [max] chars...
        fontsize = Math.floor(max*fontsize/quoteString.length);
    }

    var q2QuoteText = new PIXI.Text(textStrings["WSOTATOTSU"] + "\n", {font:fontsize+"px Cairo", fill:"#FFFFFF", align:"center"});
    q2QuoteText.anchor.x = 0.5;
    q2QuoteText.anchor.y = 0.5;
    q2QuoteText.x = Game.width / 2 + 6;
    q2QuoteText.y = Game.height / 2 + 20;
    q2.addChild(q2QuoteText);
    
    var q3Sprite = MakeSprite("quote0003");
    q3.addChild(q3Sprite);
    
    var q3Name = new PIXI.Text(textStrings["MarshallMcLuhan"], {font:"33px Cairo", fill:"#FFFFFF", align:"center"});
    q3Name.anchor.x = 0.5;
    q3Name.anchor.y = 0.5;
    q3Name.x = Game.width / 2 + 4;
    q3Name.y = Game.height / 2 + 76;
    q3.addChild(q3Name);
    
    var q4Sprite = MakeSprite("quote0004");
    q4.addChild(q4Sprite);
    
    var q4Text = new PIXI.Text(textStrings["misatrributed"] + "\n", {font:"33px Cairo", fill:"#FFFFFF", align:"center"});
    q4Text.anchor.x = 0.5;
    q4Text.anchor.y = 0.5;
    q4Text.x = Game.width / 2 + 4;
    q4Text.y = Game.height / 2 + 135;
    q4.addChild(q4Text);
    
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
