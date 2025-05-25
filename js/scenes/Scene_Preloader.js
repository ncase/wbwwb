Game.addToManifest({
	bg_preload: "sprites/bg_preload.png",
	bg_preload_2: "sprites/bg_preload_2.png",
	preload_play: "sprites/misc/preload_play.json"
}, true);

function Scene_Preloader(){
	
	var self = this;
	Scene.call(self);

	// Background!
	Game.stage.addChild(MakeSprite("bg_preload"));

	// RECURSIVE SCREEN
	var renderTexturePoolIndex = 0;
	var renderTexturePool = [
		new PIXI.RenderTexture(Game.renderer, Game.width, Game.height),
		new PIXI.RenderTexture(Game.renderer, Game.width, Game.height)
	];
	self.stream = new PIXI.Sprite();
	self.stream.x = 722;
	self.stream.y = 359;
	self.stream.scale.x = self.stream.scale.y = 0.29;
	self.stream.anchor.x = self.stream.anchor.y = 0.5;
	self.stream.rotation = 0.087263889;
	Game.stage.addChild(self.stream);

	// Background! #2
	Game.stage.addChild(MakeSprite("bg_preload_2"));

	// Loading Bar
	var bar = MakeMovieClip("preload_play");
	bar.anchor.x = bar.anchor.y = 0.5;
	bar.x = 278;
	bar.y = 250;
	Game.stage.addChild(bar);
	var barEase = 0.9;

	// Loading text
	var text = new PIXI.Text("loading... 0%", {font:"25px Cairo", fill:"#4C4C4C", align:"center"});
    text.anchor.x = 0.5;
    text.anchor.y = 0.5;
    text.x = bar.x;
    text.y = bar.y;
    Game.stage.addChild(text);
    
    // Playing time text
    var playingTimeText = new PIXI.Text(textStrings["playingTime"] + "\n", { font: "32px Cairo", fill: "#FFFFFF", align: "center" }); // \n hack. needed when the text field cuts some of the string font's bottom
    playingTimeText.anchor.x = 0.5;
    playingTimeText.anchor.y = 0.5;
    playingTimeText.x = bar.x;
    playingTimeText.y = 345;
    Game.stage.addChild(playingTimeText);
    
    // Warning text
    var warningText = new PIXI.Text(textStrings["warning"], { font: "25px Cairo", fill: "#666666", align: "center" });
    warningText.anchor.x = 0.5;
    warningText.anchor.y = 0.5;
    warningText.x = bar.x;
    warningText.y = 422;
    Game.stage.addChild(warningText);
    
    // Language Selector (PIXI elements)
    var languageContainer = new PIXI.Container();
    languageContainer.x = Game.width - 20; // 20px from right edge
    languageContainer.y = 20; // 20px from top
    Game.stage.addChild(languageContainer);
    
    // Available languages
    var languages = [
        { code: 'auto', name: 'Auto-detect', flag: '🌐' },
        { code: 'en', name: 'English', flag: '🇺🇸' },
        { code: 'fr', name: 'Français', flag: '🇫🇷' },
        { code: 'de', name: 'Deutsch', flag: '🇩🇪' },
        { code: 'fa', name: 'فارسی', flag: '🇮🇷' },
        { code: 'pt', name: 'Português', flag: '🇵🇹' },
        { code: 'pt-BR', name: 'Português (BR)', flag: '🇧🇷' },
        { code: 'es', name: 'Español', flag: '🇪🇸' },
        { code: 'zh-TW', name: '繁體中文', flag: '🇹🇼' },
        { code: 'tr', name: 'Türkçe', flag: '🇹🇷' }
    ];
    
    // Get current language
    var currentLang = navigator.language || 'en';
    var currentLangCode = currentLang.indexOf('-') > -1 ? currentLang : currentLang.split('-')[0];
    var currentLanguage = languages.find(lang => lang.code === currentLangCode) || languages.find(lang => lang.code === 'auto');
    
    var dropdownOpen = false;
    var dropdownItems = [];
    var selectorWidth = 140;
    var selectorHeight = 40;
    var itemHeight = 35;
    
    // Drop shadow for main button
    var shadowBg = new PIXI.Graphics();
    shadowBg.beginFill(0x000000, 0.2);
    shadowBg.drawRoundedRect(-selectorWidth + 2, 2, selectorWidth, selectorHeight, 8);
    shadowBg.endFill();
    languageContainer.addChild(shadowBg);
    
    // Main selector button background with gradient effect
    var selectorBg = new PIXI.Graphics();
    selectorBg.beginFill(0xF8F8F8);
    selectorBg.lineStyle(2, 0x888888, 1);
    selectorBg.drawRoundedRect(-selectorWidth, 0, selectorWidth, selectorHeight, 8);
    selectorBg.endFill();
    
    // Add subtle inner gradient
    var gradientOverlay = new PIXI.Graphics();
    gradientOverlay.beginFill(0xFFFFFF, 0.3);
    gradientOverlay.drawRoundedRect(-selectorWidth + 2, 2, selectorWidth - 4, selectorHeight/2 - 1, 6);
    gradientOverlay.endFill();
    selectorBg.addChild(gradientOverlay);
    
    languageContainer.addChild(selectorBg);
    
    // Selected language flag
    var selectedFlag = new PIXI.Text(currentLanguage.flag, {
        font: "18px Arial", 
        fill: "#333333"
    });
    selectedFlag.x = -selectorWidth + 12;
    selectedFlag.y = selectorHeight/2;
    selectedFlag.anchor.y = 0.5;
    languageContainer.addChild(selectedFlag);
    
    // Selected language text
    var selectedLangText = new PIXI.Text(currentLanguage.name, {
        font: "14px Cairo", 
        fill: "#333333", 
        align: "left"
    });
    selectedLangText.x = -selectorWidth + 35;
    selectedLangText.y = selectorHeight/2;
    selectedLangText.anchor.y = 0.5;
    languageContainer.addChild(selectedLangText);
    
    // Dropdown arrow with better styling
    var dropdownArrow = new PIXI.Text("▼", {
        font: "14px Arial", 
        fill: "#666666",
        fontWeight: "bold"
    });
    dropdownArrow.x = -18;
    dropdownArrow.y = selectorHeight/2;
    dropdownArrow.anchor.x = 0.5;
    dropdownArrow.anchor.y = 0.5;
    languageContainer.addChild(dropdownArrow);
    
    // Dropdown menu container with shadow
    var dropdownShadow = new PIXI.Graphics();
    dropdownShadow.y = selectorHeight + 7;
    dropdownShadow.visible = false; // Hide by default
    languageContainer.addChild(dropdownShadow);
    
    var dropdownMenu = new PIXI.Container();
    dropdownMenu.y = selectorHeight + 5;
    dropdownMenu.visible = false;
    languageContainer.addChild(dropdownMenu);
    
    // Create dropdown background container
    var dropdownBg = new PIXI.Graphics();
    dropdownBg.beginFill(0xFFFFFF);
    dropdownBg.lineStyle(2, 0x888888);
    dropdownBg.drawRoundedRect(-selectorWidth, 0, selectorWidth, languages.length * itemHeight, 8);
    dropdownBg.endFill();
    dropdownMenu.addChild(dropdownBg);
    
    // Create dropdown items with improved styling
    languages.forEach(function(lang, index) {
        var itemContainer = new PIXI.Container();
        itemContainer.y = index * itemHeight;
        
        // Item drop shadow
        var itemShadow = new PIXI.Graphics();
        itemShadow.beginFill(0x000000, 0.1);
        itemShadow.drawRoundedRect(-selectorWidth + 2, 2, selectorWidth, itemHeight - 2, 4);
        itemShadow.endFill();
        dropdownShadow.addChild(itemShadow);
        
        // Item background - no individual borders, just hover effect
        var itemBg = new PIXI.Graphics();
        itemBg.beginFill(0xFFFFFF, 0); // Transparent by default
        if (index === 0) {
            itemBg.drawRoundedRect(-selectorWidth + 2, 2, selectorWidth - 4, itemHeight - 2, 6, 6, 0, 0);
        } else if (index === languages.length - 1) {
            itemBg.drawRoundedRect(-selectorWidth + 2, 2, selectorWidth - 4, itemHeight - 2, 0, 0, 6, 6);
        } else {
            itemBg.drawRect(-selectorWidth + 2, 2, selectorWidth - 4, itemHeight - 2);
        }
        itemBg.endFill();
        itemContainer.addChild(itemBg);
        
        // Item separator line (except for last item)
        if (index < languages.length - 1) {
            var separator = new PIXI.Graphics();
            separator.lineStyle(1, 0xE0E0E0);
            separator.moveTo(-selectorWidth + 10, itemHeight - 1);
            separator.lineTo(-10, itemHeight - 1);
            itemContainer.addChild(separator);
        }
        
        // Item flag
        var itemFlag = new PIXI.Text(lang.flag, {
            font: "16px Arial", 
            fill: "#333333"
        });
        itemFlag.x = -selectorWidth + 12;
        itemFlag.y = itemHeight/2;
        itemFlag.anchor.y = 0.5;
        itemContainer.addChild(itemFlag);
        
        // Item text
        var itemText = new PIXI.Text(lang.name, {
            font: "13px Cairo", 
            fill: "#333333", 
            align: "left"
        });
        itemText.x = -selectorWidth + 35;
        itemText.y = itemHeight/2;
        itemText.anchor.y = 0.5;
        itemContainer.addChild(itemText);
        
        // Make interactive
        itemContainer.interactive = true;
        // itemContainer.buttonMode = true; // Removed to show only game cursor
        
        // Hover effects with smooth gradient
        itemContainer.mouseover = function() {
            itemBg.clear();
            itemBg.beginFill(0x777777, 0.1);
            itemBg.drawRect(-selectorWidth + 2, 2, selectorWidth - 4, itemHeight - 2);
            itemBg.endFill();
            itemText.style.fill = "#555555";
        };
        
        itemContainer.mouseout = function() {
            itemBg.clear();
            itemBg.beginFill(0xFFFFFF, 0); // Back to transparent
            itemBg.drawRect(-selectorWidth + 2, 2, selectorWidth - 4, itemHeight - 2);
            itemBg.endFill();
            itemText.style.fill = "#333333";
        };
        
        // Click handler with animation
        itemContainer.mousedown = itemContainer.touchend = function() {
            // Update selected language
            currentLanguage = lang;
            selectedFlag.text = lang.flag;
            selectedLangText.text = lang.name;
            
            // Change language
            var selectedLang = lang.code === 'auto' ? null : lang.code;
            initializeTextStrings(selectedLang);
            
            // Update text elements
            playingTimeText.text = textStrings["playingTime"] + "\n";
            warningText.text = textStrings["warning"];
            
            // Close dropdown with animation
            dropdownOpen = false;
            dropdownMenu.visible = false;
            dropdownShadow.visible = false;
            dropdownArrow.text = "▼";
            dropdownArrow.style.fill = "#666666";
            
            // Brief flash effect on main button
            selectorBg.tint = 0xF0F0F0;
            setTimeout(function() {
                selectorBg.tint = 0xFFFFFF;
            }, 150);
        };
        
        dropdownMenu.addChild(itemContainer);
        dropdownItems.push(itemContainer);
    });
    
    // Main selector click handler with improved feedback
    selectorBg.interactive = true;
    // selectorBg.buttonMode = true; // Removed to show only game cursor
    
    selectorBg.mouseover = function() {
        if (!dropdownOpen) {
            selectorBg.tint = 0xF0F0F0;
            dropdownArrow.style.fill = "#444444";
        }
    };
    
    selectorBg.mouseout = function() {
        if (!dropdownOpen) {
            selectorBg.tint = 0xFFFFFF;
            dropdownArrow.style.fill = "#666666";
        }
    };
    
    selectorBg.mousedown = selectorBg.touchend = function() {
        dropdownOpen = !dropdownOpen;
        dropdownMenu.visible = dropdownOpen;
        dropdownShadow.visible = dropdownOpen;
        dropdownArrow.text = dropdownOpen ? "▲" : "▼";
        dropdownArrow.style.fill = dropdownOpen ? "#444444" : "#666666";
        selectorBg.tint = dropdownOpen ? 0xF0F0F0 : 0xFFFFFF;
    };
    
    // Store references for cleanup
    self.languageContainer = languageContainer;
    
    // CURSOR
    var cursor = new Cursor(self);
    Game.stage.addChild(cursor.graphics);

	// Update!
	self.update = function(){

		// RECURSIVE SCREEN
		var renderTexture = renderTexturePool[renderTexturePoolIndex];
	    renderTexture.render(Game.stage);
	    renderTexturePoolIndex = (renderTexturePoolIndex+1)%renderTexturePool.length;
	    self.stream.texture = renderTexture;

	    // THE BUTTON!
	    var scale = bar.scale.x;
	    var gotoScale = (bar.currentFrame==2) ? 1.02 : 1.00;
	    scale = scale*barEase + gotoScale*(1-barEase);
	    bar.scale.x = bar.scale.y = scale;

	    // Ya
	    cursor.update((bar.currentFrame==2));

	};

	Game.loadAssets(function(){

		Game.stage.removeChild(text);
		bar.gotoAndStop(1);
		bar.scale.x = bar.scale.y = 1.1;

		// INTERACTIVITY!
		bar.interactive = true;
		bar.mouseover = function(){
			barEase = 0.5;
			bar.gotoAndStop(2);
		};
		bar.mouseout = function(){
			bar.gotoAndStop(1);
		};
		bar.mousedown = bar.touchend = function(){
			Game.sounds.squeak.play();
			setTimeout(function(){
				Game.sceneManager.gotoScene("Quote");
			},200);
		};

	}, function(ratio){
		var percent = Math.floor(ratio*100);
		text.text = "loading... "+percent+"%";
	}, false);

	// Cleanup function for when scene changes
	self.kill = function() {
		// PIXI elements are automatically cleaned up when the stage is cleared
		// No manual DOM cleanup needed
	};

}
