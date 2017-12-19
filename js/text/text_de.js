///////////////////////////////////
/////// TRANSLATED STRINGS ////////
///////////////////////////////////

// Umlaut codes: Ä = \xC4, Ö = \xD6, Ü = \xDC, ä = \xE4, ö = \xF6, ü = \xFC, ß = \x

var textStrings_DE = {
    // Act 1
    "niceHat"              : "Oh, h\xFCbscher Hut!"                                              ,
    "outtaHere"            : "Ja, besser schnell weg hier."                                      ,
    "getARoom"             : "Widerlich, nehmt euch ein Zimmer!"                                 ,
    "notCoolAnymore"       : "Schon geh\xF6rt? H\xFCte sind out."                                ,
    "tvOnTv"               : "Ein Fernseher! Im Fernsehen!"                                      ,
    "cricky"               : "Ein Grash\xFCpfer!"                                                ,
    "tooManyCrickets"      : "Ok, zu viele Grash\xFCpfer."                                       ,
    "normalPeep"           : "Ein gew\xF6hnlicher Typ."                                          ,
    "normalPeeps"          : "Ein paar gew\xF6hnliche Typen."                                    ,
    "wowNothing"           : "Wow! Nichts! Gar nix!"                                             ,

    // Act 2
    "crazySquareAttacks"   : "Verr\xFCcktes Quadrat greift an!"                                  ,
    "justMissed"           : "Ooooh, knapp verpasst."                                            ,
    "somethingInteresting" : "(laaangweilig *g\xE4hn*)"                                          ,
    "whoIsScreaming"       : "(*wer* schreit sie denn an?)"                                      ,
    "circleFearsSquares"   : "KREIS F\xDCRCHTET QUADRATE!"                                       ,
    "whoScaresThem"        : "(*wen* f\xFCrchten sie denn?)"                                     ,
    "areTheyScared"        : "(erwische sie, *wenn* sie fl\xFCchten)"                            ,
    "squaresSnubCircles"   : "QUADRAT: Anschuldigungen von KREISEN l\xE4cherlich."               ,
    "areTheySnubbed"       : "(erwische sie, *w\xE4hrend* sie angreifen)"                        ,
    "everyoneHates"        : "ALLE HASSEN JEDEN!!1!"                                             ,
    "almostEveryoneHates"  : "FAST JEDER HASST ALLE..."                                          ,
    "squaresHateCircles"   : "QUADRATE HASSEN KREISE"                                            ,
    "circlesHateSquares"   : "KREISE HASSEN QUADRATE"                                            ,
    "areTheyYelling"       : "(erwische sie, *w\xE4hrend* sie sich gegenseitig anschreien)"      ,
    "nerdsNow"             : "Was machen diese Nerds jetzt?"                                     ,
    "schockedPeep"         : "Warum ist er geschockt?"                                           ,
    "whatever"             : "Egal..."                                                           ,

    // Act 3
    "ellipsis"             : ". . ."                                                             ,
    "coolNoMore"           : "H\xFCte sind out. Hast du's immer noch nicht geh\xF6rt?"           ,
    "beScared"             : "HABT ANGST! SEID W\xDCTEND!"                                        ,

    // MANIFESTO
    "manifesto" : [
        //"Als ob ihr an guten Nachrichten interessiert seid."     ,
        "Wer will denn sehen, wie Leute miteinender klarkommen?" ,
        "Frieden ist langweilig. Die Leute wollen Gewalt sehen." ,
        //"Frieden ist langweilig. Konflikt bringt Klicks."        ,
        "Jede gute Story braucht einen Konflikt, also..."        ,
        //"...GIB DEM PUBLIKUM WAS ES WILL!"                       ,
        "GIB DEM PUBLIKUM WAS ES WILL!"
    ] ,

    // -- Strings currently in graphics resources --

    // (bg_preload.png) Preload screen
    "playingTime"        : "Spielzeit: 5 Minuten" ,
    "warning"            : "Warnung: die folgende Sendung\nenth\xE4lt Darstellungen von Arroganz,\nBeleidigungen & Gewalt.\nund ist nicht f\xFCr Kinder geeignet." ,

    // (cam-instructions.png) Camera Instructions
    "pointAndClick"      : "POINT & CLICK" , // I'd leave that in English, even for the German translation. /stefan

    // (chyron3.png)
    "chyronNothing"      : "Oh wow, *NICHTS*!" ,

    // (credits0001.png)
    "createdBy"          : "created by" , // I'd leave that in English, even for the German translation. /stefan
    "NickyCase"          : "Nicky Case" , // name; just here in case you want to display text instead of graphic. /stefan

    // (credits0002.png)
    "manyThanks"         : "Vielen Dank an alle Tester:" ,

    // (credits000[3,4,5,6].png)
    "patreonSupporters"  : "und an meine Patreon Unterst\xFCtzer:" ,

    // (credits0007.png)
    "lastButNotLeast"    : "und zu guter Letzt," ,

    // (credits0008.png)
    "thankYouForPlaying" : "Danke an DICH f\xFCr's spielen!" ,

    // (preload_play.png)
    "playButton"         : "START" ,

    // (end_button.png) Post Credits
    "otherWorkButton"    : "andere Projekte" ,
    "buyCoffeeButton"    : "kauf mir einen Kaffee" ,
    "replayButton"       : "noch mal spielen" ,

    // (logo.png) Post Credits
    "logoWBWWB"          : "TEILE DEINEN SCHMERZ:" ,

    // (end_prototype.png)
    "endOfPrototype"     : "ENDE DES PROTOTYPS" ,
    "toBeContinued"      : "(wird fortgesetzt!)" ,

    // (quote0002.png)
    "WBWWB"              : "We become what we behold." , // I'd leave that in English, even for the German translation. /stefan
    "WSOTATOTSU"         : "Wir formen unsere Werkzeuge, und dann formen unsere Werkzeuge uns." ,

    // (quote0003.png)
    "MarshallMcLuhan"    : "Marshall McLuhan" , // name; just here in case you want to display text instead of graphic. /stefan

    // (quote0004.png)
    "misatrributed"      : "(falsch zugeschrieben)" ,
};

//                                   base_texts      translation
var textStrings = Object.assign({}, textStrings_EN, textStrings_DE);
