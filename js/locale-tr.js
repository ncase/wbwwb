/*
 * Turkish localization quality layer.
 *
 * The main locale system remains unchanged. This file provides the reviewed
 * Turkish wording and wraps WBWWB_SET_LOCALE so the reviewed strings remain
 * active if the language is changed at runtime.
 *
 * The wording is standard Turkish rather than a literal word-for-word
 * translation; punctuation, capitalization and idioms are adapted for
 * natural Turkish while preserving the original game's tone.
 */
(function (global) {
  "use strict";

  var TR = {
    niceHat: "VAY! NE GÜZEL ŞAPKA!",
    outtaHere: "Evet, hadi buradan git.",
    getARoom: "İĞRENÇ! GİDİN BİR ODA TUTUN!",
    notCoolAnymore: "Şapkalar artık havalı değil.",
    tvOnTv: "BİR TELEVİZYON... TELEVİZYONDA!",
    cricky: "KÜÇÜK CIRCIR BÖCEĞİ <3",
    tooManyCrickets: "Tamam, bu kadar cırcır böceği fazla.",
    normalPeep: "Sadece sıradan bir insan.",
    normalPeeps: "Sadece birkaç sıradan insan.",
    wowNothing: "VAY CANINA, HİÇBİR ŞEY YOK!",
    crazySquareAttacks: "ÇILGIN KARE SALDIRISI!",
    justMissed: "Ooooh, az kalsın yakalıyordun.",
    somethingInteresting: "(Onları *ilginç bir şey* yaparken yakalamalısın...)",
    whoIsScreaming: "(Onlara kimin bağırdığını yakalamalısın.)",
    circleFearsSquares: "DAİRELER KARELERDEN KORKUYOR",
    whoScaresThem: "(Ayrıca *kimden* korktuklarını da yakalamalısın.)",
    areTheyScared: "(Onları bir kareden *korkarken* yakalamalısın.)",
    squaresSnubCircles: "KARELER DAİRELERİ KÜÇÜMSÜYOR",
    areTheySnubbed: "(Onları bir daireyi *küçümserken* yakalamalısın.)",
    everyoneHates: "HERKES HERKESTEN NEFRET EDİYOR!!1!",
    almostEveryoneHates: "NEREDEYSE HERKES HERKESTEN NEFRET EDİYOR...",
    squaresHateCircles: "KARELER DAİRELERDEN NEFRET EDİYOR",
    circlesHateSquares: "DAİRELER KARELERDEN NEFRET EDİYOR",
    areTheyYelling: "(Onları başkalarına *bağırırken* yakalamalısın.)",
    nerdsNow: "Bu inekler şimdi ne yapıyor?",
    schockedPeep: "Bu insan neden şaşkın?",
    whatever: "Her neyse.",
    ellipsis: ". . .",
    coolNoMore: "Şapkaların artık havalı olmadığını duymadın mı?",
    beScared: "KORK. ÖFKELEN.",
    manifesto: [
      "Kim *insanların iyi geçinmesini* izlemek ister ki?",
      "Barış sıkıcıdır. Şiddet gündem olur.",
      "Ve her hikâyenin bir çatışmaya ihtiyacı vardır, yani...",
      "İZLEYİCİYE İSTEDİĞİNİ VER."
    ],
    playingTime: "oynama süresi: 5 dakika",
    warning: "uyarı: aşağıdaki programda\nkibir, kabalık ve toplu cinayet\nsahneleri bulunmaktadır.\nizleyicinin dikkatli olması önerilir.",
    pointAndClick: "İŞARET ET VE TIKLA",
    chyronNothing: "VAY CANINA, *HİÇBİR ŞEY* YOK",
    createdBy: "yapımcı:",
    manyThanks: "oyunumu test edenlere çok teşekkürler:",
    patreonSupporters: "ve Patreon destekçilerime:",
    lastButNotLeast: "ve son olarak,",
    thankYouForPlaying: "oynadığın için TEŞEKKÜR EDERİM!",
    playButton: "OYNA",
    otherWorkButton: "diğer çalışmalarımı gör",
    buyCoffeeButton: "bana bir kahve ısmarla",
    replayButton: "bu karmaşayı yeniden oyna",
    logoWBWWB: "ACIYI PAYLAŞ:",
    endOfPrototype: "PROTOTİPİN SONU",
    toBeContinued: "(devam edecek!)",
    WBWWB: "Gördüklerimize dönüşürüz.",
    WSOTATOTSU: "Araçlarımızı biz şekillendiririz, sonra araçlarımız da bizi şekillendirir.",
    misatrributed: "(yanlış atfedilmiştir)"
  };

  function applyTurkish() {
    if (global.WBWWB_LOCALE !== "tr" || !global.textStrings) return;
    Object.keys(TR).forEach(function (key) {
      global.textStrings[key] = TR[key];
    });
  }

  applyTurkish();

  if (typeof global.WBWWB_SET_LOCALE === "function") {
    var originalSetLocale = global.WBWWB_SET_LOCALE;
    global.WBWWB_SET_LOCALE = function (locale) {
      var selected = originalSetLocale(locale);
      applyTurkish();
      return selected;
    };
  }
})(window);
