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
    var createdByText = new PIXI.Text(textStrings["createdBy"], {font: "46px Times", fill:"#FFFFFF", align: "left"});
    createdByText.anchor.x = 0.0;
    createdByText.anchor.y = 0.5;
    createdByText.x = Game.width / 2 - 240;
    createdByText.y = Game.height / 2 - 41;
    c[1].addChild(createdByText);
    
    var authorText = new PIXI.Text(textStrings["NickyCase"], {font: "86px Times", fill:"#FFFFFF", align: "left"});
    authorText.anchor.x = 0.0;
    authorText.anchor.y = 0.5;
    authorText.x = Game.width / 2 - 240;
    authorText.y = Game.height / 2 + 29;
    c[1].addChild(authorText);
    
    var playtestersText = new PIXI.Text(textStrings["manyThanks"], {font: "44px Times", fill:"#FFFFFF", align: "right"});
    playtestersText.anchor.x = 1.0;
    playtestersText.anchor.y = 0.5;
    playtestersText.x = Game.width / 2 + 262;
    playtestersText.y = Game.height / 2 - 140;
    c[2].addChild(playtestersText);
    
    var supportersText = new PIXI.Text(textStrings["patreonSupporters"], {font: "44px Times", fill:"#AAAAAA", align: "left"});
    supportersText.anchor.x = 0.0;
    supportersText.anchor.y = 0.5;
    supportersText.x = Game.width / 2 - 0;
    supportersText.y = Game.height / 2 - 0;
    c[3].addChild(supportersText);
//    c[4].addChild(supportersText);
//    c[5].addChild(supportersText);
//    c[6].addChild(supportersText);
    
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
