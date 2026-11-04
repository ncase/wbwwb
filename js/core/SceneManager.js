/************************************

SCENE MANAGER
Basically just swaps out scenes.

*************************************/

function SceneManager(){

	var self = this;

	self.gotoScene = function(sceneName){

		// Old scene
		Game.stage.removeChildren();
		// Discard references to scene-owned adaptive layout helpers.
		Game._baseLayouts = [];
		Game._viewportFills = [];
		Game._baseAnchors = [];
		var oldScene = Game.scene;
		if(oldScene) oldScene.kill();

		// New scene
		var Scene_Class = window["Scene_"+sceneName];
		var newScene = new Scene_Class();
		Game.scene = newScene;

	};

	self.update = function(){
		if(Game.scene){
			Game.scene.update();
		}
	};

}