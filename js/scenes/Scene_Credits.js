Game.addToManifest({

	credits0001: "sprites/credits/credits0001.png", // nicky case
	credits0002: "sprites/credits/credits0002.png", // playtesters
	credits0003: "sprites/credits/credits0003.png", // patreon
	credits0004: "sprites/credits/credits0004.png", // patreon
	credits0005: "sprites/credits/credits0005.png", // patreon
	credits0006: "sprites/credits/credits0006.png", // patreon
	credits0007: "sprites/credits/credits0007.png", // and thank...
	credits0008: "sprites/credits/credits0008.png", // ...YOU!

});

function Scene_Credits(){
	
	var self = this;
	Scene.call(self);

	// Layers, yo.
	var cont = new PIXI.Container();
	Game.stage.addChild(cont);
	var c = {};
	for(var i=1; i<=8; i++){
        c[i] = new PIXI.Container();
        c[i].addChild(MakeSprite("credits000"+i));
        c[i].alpha = 0;
		cont.addChild(c[i]);
	}
    
    // add text
    var createdByText = new PIXI.Text(textStrings["createdBy"] + "\n", {font: "46px Cairo", fill:"#FFFFFF", align: "left"});
    createdByText.anchor.x = 0.0;
    createdByText.anchor.y = 0.5;
    createdByText.x = Game.width / 2 - 240;
    createdByText.y = Game.height / 2 - 41;
    c[1].addChild(createdByText);
    
    var authorText = new PIXI.Text(textStrings["NickyCase"], {font: "86px Cairo", fill:"#FFFFFF", align: "left"});
    authorText.anchor.x = 0.0;
    authorText.anchor.y = 0.5;
    authorText.x = Game.width / 2 - 240;
    authorText.y = Game.height / 2 + 29;
    c[1].addChild(authorText);
    
    var playtestersText = new PIXI.Text(textStrings["manyThanks"] + "\n", {font: "44px Cairo", fill:"#FFFFFF", align: "right"});
    playtestersText.anchor.x = 1.0;
    playtestersText.anchor.y = 0.5;
    playtestersText.x = Game.width / 2 + 262;
    playtestersText.y = Game.height / 2 - 140;
    c[2].addChild(playtestersText);
    
    // @TODO: Yes, I feel physical pain hacking this in the way I did in the following lines,
    // but I don't know PIXI enough to do this properly... /sl
	// Alex: Nicky, hold my beer. I can top this hack.
    
    var supportersText1 = new PIXI.Text(textStrings["patreonSupporters"] + "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\nasdfasdfasdfsadfasdfasdfasdfasdf", {font: "40px Cairo", fill:"#FFFFFF", align: "left"});
    supportersText1.anchor.x = 0.0;
    supportersText1.x = 145;
    supportersText1.y = 80;
    c[3].addChild(supportersText1);
    
    var supportersText2 = new PIXI.Text(textStrings["patreonSupporters"] + "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\nasdfasdfasdfsadfasdfasdfasdfasdf", {font: "40px Cairo", fill:"#FFFFFF", align: "left"});
    supportersText2.anchor.x = 0.0;
    supportersText2.x = 145;
    supportersText2.y = 80;
    c[4].addChild(supportersText2);
    
    var supportersText3 = new PIXI.Text(textStrings["patreonSupporters"] + "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\nasdfasdfasdfsadfasdfasdfasdfasdf", {font: "40px Cairo", fill:"#FFFFFF", align: "left"});
    supportersText3.anchor.x = 0.0;
    supportersText3.x = 145;
    supportersText3.y = 80;
    c[5].addChild(supportersText3);
    
    var supportersText4 = new PIXI.Text(textStrings["patreonSupporters"] + "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\nasdfasdfasdfsadfasdfasdfasdfasdf", {font: "40px Cairo", fill:"#FFFFFF", align: "left"});
    supportersText4.anchor.x = 0.0;
    supportersText4.x = 145;
    supportersText4.y = 80;
    c[6].addChild(supportersText4);
    
    // thankYouText comes before finallyText so finallyText can be aligned based on thankYouText. /sl
    var thankYouText = new PIXI.Text(textStrings["thankYouForPlaying"] + "\n   ", {font: "55px Cairo", fill:"#FFFFFF", align: "center"});
    thankYouText.anchor.x = 0.5;
    thankYouText.anchor.y = 0.5;
    thankYouText.x = Game.width / 2 + 10;
    thankYouText.y = Game.height / 2 + 25;
    c[8].addChild(thankYouText);
    
    var finallyText = new PIXI.Text(textStrings["lastButNotLeast"] + "\n", {font: "37px Cairo", fill:"#FFFFFF", align: "left"});
    finallyText.anchor.x = 0.0;
    finallyText.anchor.y = 0.5;
    finallyText.x = thankYouText.getBounds().x + thankYouText.width - finallyText.width;
    finallyText.y = Game.height / 2 - 45;
    c[7].addChild(finallyText);
    
	// TWEEN ANIM
	Tween_get(c[1]).wait(_s(BEAT*4)) // 0. Wait 4 beats before credits...
	.to({alpha:1}, _s(BEAT), Ease.quadInOut) // 1. CREATED BY!
	.wait(_s(BEAT*3))
	.to({alpha:0}, _s(BEAT), Ease.quadInOut)
	.call(function(){

		// 2. PLAYTESTERS
		Tween_get(c[2])
		.to({alpha:1}, _s(BEAT), Ease.quadInOut)
		.wait(_s(BEAT*3))
		.to({alpha:0}, _s(BEAT), Ease.quadInOut)
		.call(function(){

			// 3. PATREONS: CUT BETWEEN THEM THEN FADE OUT
			Tween_get(c[3])
			.to({alpha:1}, _s(BEAT), Ease.quadInOut)
			.wait(_s(BEAT*2))
			.call(function(){

				// CUT!
				c[3].alpha = 0;
				c[4].alpha = 1;

				Tween_get(c[4])
				.wait(_s(BEAT*2))
				.call(function(){

					// CUT!
					c[4].alpha = 0;
					c[5].alpha = 1;

					Tween_get(c[5])
					.wait(_s(BEAT*2))
					.call(function(){

						// CUT!
						c[5].alpha = 0;
						c[6].alpha = 1;

						Tween_get(c[6])
						.wait(_s(BEAT*2))
						.to({alpha:0}, _s(BEAT), Ease.quadInOut) // fade...
						.call(function(){

							// 4. And finally... thank YOU!
							Tween_get(c[7])
							.to({alpha:1}, _s(BEAT), Ease.quadInOut)
							.wait(_s(BEAT))
							.call(function(){
								Tween_get(c[8])
								.to({alpha:1}, _s(BEAT), Ease.quadInOut)
								.wait(_s(BEAT*3))
								.call(function(){
									
									// 5. Fade everything out, and NIGHTTIME SOUNDS
									Tween_get(cont)
									.wait(_s(BEAT))
									.to({alpha:0}, _s(BEAT), Ease.quadInOut)
									.call(function(){
										Game.sceneManager.gotoScene("Post_Credits");
									});

									// Background Ambience
									var ambience = Game.sounds.bg_nighttime;
								   	ambience.loop(true);
								   	ambience.volume(0);
								   	ambience.play();
								   	ambience.fade(0, 1, 2000);

								});	
							});

						});
					});
				});
			});

		});

	});

}
