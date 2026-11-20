# We Become What We Scroll

*a tiny game about feeds, likes, and vicious cycles* - a social media remix of Nicky Case's **We Become What We Behold**.

Instead of a news camera and a TV, you hold a phone. Point it at the peeps in the park and click to **post**. Whatever you post shows up on the giant phone-on-a-stand, with a caption, a TRENDING tag and a like counter. The peeps who see the post copy it, fear it, or get angry about it... and the feed only rewards conflict.

**Play locally:** serve the folder with any static server (e.g. `npx serve .` or `python -m http.server`) and open `index.html`. WebGL is required.

**What changed from the original:**
- `js/game/TV.js` - the TV is now a phone on a stand; chyrons became social posts (handle, TRENDING tag, caption, animated likes & shares)
- `js/game/Director.js` - likes are calculated from how many peeps the post reaches
- `js/textStrings.js` - new `textStrings_SOCIAL` language with social-media captions and ending; set as the active language
- `js/game/Camera.js` - "POINT & POST" instructions
- `js/scenes/Scene_Preloader.js` - new title, "WE BECOME WHAT WE SCROLL"

All characters, art, sounds and engine code come from the original game, which Nicky Case released to the public domain (CC0). Original credits below.

---

# DEAR ASPIRING TRANSLATORS, PLEASE CHECK THE MAIN PAGE DESCRIPTION FIRST TO SEE IF YOUR TRANSLATION ALREADY EXISTS: https://ncase.itch.io/wbwwb (also check the Issues to see if your translation is already being worked on - collaborate with them if they're stuck!)

*a game about news cycles, vicious cycles, infinite cycles*

**[PLAY](https://ncase.itch.io/wbwwb)**

---

### Made with open culture, for open culture!

I'm releasing all my code and art to the public domain, under the [Creative Commons Zero](http://creativecommons.org/publicdomain/zero/1.0/) un-license. Which means if you wanna remix this to make your own way-too-meta game, or use it in a presentation or classroom or whatever, you already have my permission!

However, not *all* the code/art is mine. Credit's due where credit's due, so...

**CODE:**    
- [PIXI.js](https://github.com/pixijs/pixi.js), for rendering the graphics (MIT License)    
- [Howler.js](https://github.com/goldfire/howler.js), for playing the sounds (MIT License)

**SOUNDS:**    
- [squeak!](https://www.freesound.org/people/ermfilm/sounds/130011/) (CC BY)    
- [park ambience](https://www.freesound.org/people/Mafon2/sounds/274175/) (CC Zero)    
- [camera shutter](https://www.freesound.org/people/uEffects/sounds/207865/) (CC Zero)    
- [single cricket](https://www.freesound.org/people/cs272/sounds/77034/) (CC-BY)    
- [multiple crickets](https://www.freesound.org/people/alienistcog/sounds/124583/) (CC Zero)    
- [news jingle](https://www.freesound.org/people/Tuben/sounds/272044/) (CC Zero)    
- [scream #1](https://www.freesound.org/people/GreatNate98/sounds/353086/) (CC Zero)    
- [scream #2](https://www.freesound.org/people/mariallinas/sounds/222649/) (CC Zero)    
- [gunshot](https://www.freesound.org/people/mitchelk/sounds/136766/) (CC Zero)    
- [gun cocked](https://www.freesound.org/people/martian/sounds/182229/) (CC Zero)    
- [shotgun](https://www.freesound.org/people/lensflare8642/sounds/145209/) (CC Zero)        
- [bloody impact](https://www.freesound.org/people/Hybrid_V/sounds/319590/) (CC BY)        
- [creepy warp sound](https://www.freesound.org/people/Andromadax24/sounds/184476/) (CC BY)        
- [crowd screaming](https://www.freesound.org/people/MultiMax2121/sounds/156860/) (CC Zero)        

**ART:**    
- For the ending, I modified [this photo of a laptop](https://unsplash.com/photos/XyNi3rUEReE). (CC Zero)
