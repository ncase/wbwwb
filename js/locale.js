/*
 * Locale selection and fallback.
 *
 * Order:
 *   1. ?lang=xx or ?locale=xx
 *   2. localStorage["wbwwb.locale"]
 *   3. navigator.languages / navigator.language
 *   4. English
 *
 * Locale aliases are normalized so values such as tr-TR and pt-BR work.
 */
(function (global) {
  "use strict";

  var EN = global.WBWWB_EN;
  var overrides = {
    de: {
      niceHat: "OH, HÜBSCHER HUT!", outtaHere: "Ja, besser schnell weg hier.", getARoom: "WIDERLICH, NEHMT EUCH EIN ZIMMER!", notCoolAnymore: "Hüte sind schon wieder out.", tvOnTv: "EIN FERNSEHER... IM FERNSEHEN!", cricky: "EIN KLEINER GRASHÜPFER <3", tooManyCrickets: "Okay, das sind zu viele Grashüpfer.", normalPeep: "nur ein gewöhnlicher Typ", normalPeeps: "nur ein paar gewöhnliche Typen", wowNothing: "WOW! NICHTS!", crazySquareAttacks: "VERRÜCKTES QUADRAT GREIFT AN!", justMissed: "Ooooh, knapp verpasst.", circleFearsSquares: "KREIS FÜRCHTET QUADRATE", squaresSnubCircles: "QUADRATE IGNORIEREN KREISE", everyoneHates: "ALLE HASSEN JEDEN!!1!", almostEveryoneHates: "FAST JEDER HASST ALLE...", squaresHateCircles: "QUADRATE HASSEN KREISE", circlesHateSquares: "KREISE HASSEN QUADRATE", whatever: "Egal...", coolNoMore: "Hüte sind out. Hast du das noch nicht gehört?", beScared: "HABT ANGST! SEID WÜTEND!", playingTime: "Spielzeit: 5 Minuten", warning: "Warnung: Die folgende Sendung\nenthält Darstellungen von Arroganz,\nBeleidigungen und Gewalt.\nZuschauerdisziplin wird empfohlen.", pointAndClick: "POINT & CLICK", playButton: "START", otherWorkButton: "andere Projekte", buyCoffeeButton: "kauf mir einen Kaffee", replayButton: "noch mal spielen", endOfPrototype: "ENDE DES PROTOTYPS", toBeContinued: "(wird fortgesetzt!)", WSOTATOTSU: "Wir formen unsere Werkzeuge, und dann formen unsere Werkzeuge uns.", misatrributed: "(falsch zugeschrieben)"
    },
    es: {
      niceHat: "OH, QUÉ BUEN SOMBRERO", outtaHere: "Sí, vete de aquí.", getARoom: "QUÉ ASCO, ¡VAYAN A UNA HABITACIÓN!", notCoolAnymore: "los sombreros ya no molan", tvOnTv: "¡UNA TV... EN UNA TV!", cricky: "PEQUEÑO GRILLO <3", tooManyCrickets: "ok, son demasiados grillos", normalPeep: "solo una persona normal", normalPeeps: "solo unas personas normales", wowNothing: "¡WOW, NO HAY NADA!", crazySquareAttacks: "EL CUADRADO LOCO ATACA", justMissed: "oooooh, casi lo pillo", circleFearsSquares: "LOS CÍRCULOS TEMEN A LOS CUADRADOS", squaresSnubCircles: "LOS CUADRADOS DESPRECIAN A LOS CÍRCULOS", everyoneHates: "¡TODOS ODIAN A TODOS!", almostEveryoneHates: "CASI TODOS ODIAN A TODOS...", squaresHateCircles: "LOS CUADRADOS ODIAN A LOS CÍRCULOS", circlesHateSquares: "LOS CÍRCULOS ODIAN A LOS CUADRADOS", nerdsNow: "¿qué hacen ahora esos nerds?", whatever: "lo que sea", coolNoMore: "los sombreros ya no molan, ¿no te has enterado?", beScared: "TEN MIEDO. ENFÁDATE.", playingTime: "tiempo de juego: 5 minutos", warning: "advertencia: el siguiente programa\ncontiene escenas de arrogancia,\ngrosería y asesinato en masa.\nse recomienda discreción.", pointAndClick: "APUNTA Y HAZ CLIC", playButton: "JUGAR", otherWorkButton: "ver mis otros trabajos", buyCoffeeButton: "invítame a un café", replayButton: "jugar de nuevo", endOfPrototype: "FIN DEL PROTOTIPO", toBeContinued: "(¡continuará!)", WBWWB: "Nos convertimos en lo que contemplamos.", WSOTATOTSU: "Damos forma a nuestras herramientas y luego nuestras herramientas nos dan forma.", misatrributed: "(atribuido erróneamente)"
    },
    tr: {
      niceHat: "VAY! NE GÜZEL ŞAPKA!", outtaHere: "evet, çık git buradan", getARoom: "İĞRENÇ, GİDİN BİR ODA TUTUN", notCoolAnymore: "şapkalar artık havalı değil", tvOnTv: "TELEVİZYON... TELEVİZYON İÇİNDE!", cricky: "KÜÇÜK CIRCIR <3", tooManyCrickets: "tamam, bu kadar cırcır böceği çok fazla", normalPeep: "sadece normal bir insan", normalPeeps: "sadece normal insanlar", wowNothing: "VAYY, HİÇBİR ŞEY YOK", crazySquareAttacks: "ÇILGIN KARE SALDIRIYOR", justMissed: "of, az farkla kaçırdın", somethingInteresting: "(onları *ilginç bir şey* yaparken yakalamalısın...)", whoIsScreaming: "(onlara kimin bağırdığını yakalamalısın)", circleFearsSquares: "DAİRE KARELERDEN KORKUYOR", whoScaresThem: "(ayrıca *kimden* korktuklarını da yakalamalısın)", areTheyScared: "(onları bir kareden *korkarken* yakalamalısın)", squaresSnubCircles: "KARELER DAİRELERİ AŞAĞILIYOR", areTheySnubbed: "(onları bir daireyi *aşağılarken* yakalamalısın)", everyoneHates: "HERKES HERKESTEN NEFRET EDİYOR!!1!", almostEveryoneHates: "NEREDEYSE HERKES HERKESTEN NEFRET EDİYOR...", squaresHateCircles: "KARELER DAİRELERDEN NEFRET EDİYOR", circlesHateSquares: "DAİRELER KARELERDEN NEFRET EDİYOR", areTheyYelling: "(onları diğerlerine *bağırırken* yakalamalısın)", nerdsNow: "bu inekler şimdi ne yapıyor?", schockedPeep: "bu insan neden şok oldu?", whatever: "her neyse", ellipsis: ". . .", coolNoMore: "şapkaların artık havalı olmadığını duymadın mı?", beScared: "KORK. ÖFKELEN.", manifesto: ["kim *insanların iyi anlaştığını* izlemek ister ki?", "barış sıkıcıdır. şiddet gündem olur.", "ve her hikâye bir çatışmaya ihtiyaç duyar, yani...", "İZLEYİCİYE İSTEDİĞİNİ VER."], playingTime: "oyun süresi: 5 dakika", warning: "uyarı: bu program\nkibirlilik, kabalık ve toplu cinayet\nsahneleri içerir.\nizleyici takdiri tavsiye edilir.", pointAndClick: "İŞARET ET & TIKLA", chyronNothing: "VAYY, *HİÇBİR ŞEY* YOK", createdBy: "yaratıcı", manyThanks: "test oyuncularıma çok teşekkürler:", patreonSupporters: "ve Patreon destekçilerime:", lastButNotLeast: "ve son olarak,", thankYouForPlaying: "oynadığın için SANA teşekkürler!", playButton: "OYNA", otherWorkButton: "diğer çalışmalarımı gör", buyCoffeeButton: "bana bir kahve ısmarla", replayButton: "bu karmaşayı tekrar oyna", logoWBWWB: "ACIYI PAYLAŞ:", endOfPrototype: "PROTOTİP SONU", toBeContinued: "(devam edecek!)", WBWWB: "Baktıklarımız oluruz.", WSOTATOTSU: "Aletlerimizi biz şekillendiririz ve sonra aletlerimiz bizi şekillendirir.", misatrributed: "(yanlış bir şekilde ona atfedilir)"
    },
    pt: {
      niceHat: "OH, BELO CHAPÉU", outtaHere: "sim, saia daqui", getARoom: "ECA! VÁ FAZER ISSO NUM QUARTO", notCoolAnymore: "chapéus não são mais legais", tvOnTv: "UMA TV... NA TV!", cricky: "UM PEQUENO GRILO <3", tooManyCrickets: "ok, agora são MUITOS grilos", normalPeep: "apenas uma pessoa comum", normalPeeps: "apenas algumas pessoas comuns", wowNothing: "AAAAA, NÃO É NADA", crazySquareAttacks: "UM QUADRADO MALUCO ATACOU", justMissed: "oooooh, perdi isso!", somethingInteresting: "(você tem que pegá-los fazendo *algo* interessante...)", whoIsScreaming: "(você tem que pegar quem está gritando com eles)", circleFearsSquares: "CÍRCULOS TEMEM QUADRADOS", whoScaresThem: "(você também tem que pegar *quem* está assustando eles)", areTheyScared: "(você tem que pegar eles *sendo* assustados por um quadrado)", squaresSnubCircles: "QUADRADOS AFRONTAM CÍRCULOS", everyoneHates: "TODO MUNDO ODEIA TODO MUNDO!!1!", almostEveryoneHates: "QUASE TODO MUNDO ODEIA TODO MUNDO...", squaresHateCircles: "QUADRADOS ODEIAM CÍRCULOS", circlesHateSquares: "CÍRCULOS ODEIAM QUADRADOS", nerdsNow: "o que esses nerds estão fazendo agora?", whatever: "tanto faz", coolNoMore: "chapéus não são mais legais, você não soube?", beScared: "FIQUE COM MEDO! FIQUE COM RAIVA.", playingTime: "tempo de jogo: 5 minutos", warning: "atenção: este programa\ncontém cenas de afrontas,\ngrosseria e assassinatos em massa.\naconselha-se discrição do espectador.", pointAndClick: "POINT & CLICK", playButton: "JOGAR", otherWorkButton: "veja meus outros trabalhos", buyCoffeeButton: "me pague um café", replayButton: "jogue novamente", logoWBWWB: "COMPARTILHE:", endOfPrototype: "FIM DO PROTÓTIPO", toBeContinued: "(continua!)", WBWWB: "Nós nos tornamos o que vemos.", WSOTATOTSU: "Nós moldamos nossas ferramentas e então nossas ferramentas nos moldam.", misatrributed: "(atribuído erroneamente)"
    },
    fa: {
      niceHat: "به‌به! چه کلاه خفنی!", outtaHere: "برو گمشو بیرون!", notCoolAnymore: "دیگه کلاه خز شده!", tvOnTv: "یک تلویزیون... توی تلویزیون!", cricky: "یک جیرجیرک کوچولو <3", tooManyCrickets: "خب، دیگه جیرجیرک خیلی زیاده", normalPeep: "فقط یک آدم معمولی", normalPeeps: "فقط چند آدم معمولی", wowNothing: "واو، هیچ‌چی نیست!", crazySquareAttacks: "حمله مربع‌های دیوانه!", justMissed: "اووووه، نزدیک بود!", circleFearsSquares: "دایره از مربع می‌ترسد", squaresSnubCircles: "مربع‌ها دایره‌ها را تحقیر می‌کنند", everyoneHates: "همه از همه متنفرند!!1!", almostEveryoneHates: "تقریباً همه از همه متنفرند...", squaresHateCircles: "مربع‌ها از دایره‌ها متنفرند", circlesHateSquares: "دایره‌ها از مربع‌ها متنفرند", whatever: "هرچی", coolNoMore: "شنیدی می‌گن کلاه دیگه خز شده؟", beScared: "بترس. عصبانی شو.", playingTime: "زمان بازی: ۵ دقیقه", warning: "هشدار: این برنامه\nشامل صحنه‌هایی از خشونت،\nبی‌ادبی و کشتار جمعی است.\nتماشاگر احتیاط کند.", pointAndClick: "نشانه بگیر و کلیک کن", playButton: "بازی", replayButton: "دوباره بازی کن", endOfPrototype: "پایان نمونه اولیه", toBeContinued: "(ادامه دارد!)"
    }
  };

  var supported = ["en", "de", "es", "tr", "pt", "fa"];

  function normalize(value) {
    if (!value) return null;
    value = String(value).toLowerCase().replace("_", "-");
    var base = value.split("-")[0];
    return supported.indexOf(value) >= 0 ? value : (supported.indexOf(base) >= 0 ? base : null);
  }

  function queryLocale() {
    try {
      var params = new URLSearchParams(global.location.search);
      return normalize(params.get("lang") || params.get("locale"));
    } catch (_) {
      return null;
    }
  }

  function storedLocale() {
    try { return normalize(global.localStorage.getItem("wbwwb.locale")); } catch (_) { return null; }
  }

  function browserLocale() {
    var languages = global.navigator.languages || [];
    languages = languages.concat(global.navigator.language || []);
    for (var i = 0; i < languages.length; i++) {
      var locale = normalize(languages[i]);
      if (locale) return locale;
    }
    return null;
  }

  function buildLocale(locale) {
    var result = {};
    Object.keys(EN).forEach(function (key) { result[key] = EN[key]; });
    if (locale !== "en" && overrides[locale]) {
      Object.keys(overrides[locale]).forEach(function (key) {
        if (Object.prototype.hasOwnProperty.call(EN, key)) result[key] = overrides[locale][key];
      });
    }
    return result;
  }

  function setLocale(requested) {
    var locale = normalize(requested) || "en";
    global.textStrings = buildLocale(locale);
    global.WBWWB_LOCALE = locale;
    try { global.localStorage.setItem("wbwwb.locale", locale); } catch (_) {}
    return locale;
  }

  var selected = queryLocale() || storedLocale() || browserLocale() || "en";
  setLocale(selected);
  global.WBWWB_LOCALES = supported.slice();
  global.WBWWB_SET_LOCALE = setLocale;
})(window);
