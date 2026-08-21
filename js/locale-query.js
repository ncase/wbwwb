/*
 * Explicit URL locale override.
 *
 * This is intentionally separate from the general locale detector so that
 * ?lang=xx / ?locale=xx is ALWAYS authoritative, even when an older locale
 * was saved in localStorage or a stale browser preference exists.
 *
 * Runtime/translation infrastructure only; scenes and assets are untouched.
 */
(function (global) {
  "use strict";

  function normalize(value) {
    if (!value) return null;
    value = String(value).toLowerCase().replace(/_/g, "-");
    return value.split("-")[0];
  }

  var requested = null;
  try {
    var params = new URLSearchParams(global.location.search);
    requested = normalize(params.get("lang") || params.get("locale"));
  } catch (_) {}

  if (!requested) return;

  var supported = ["en", "de", "es", "tr", "pt"];
  var locale = supported.indexOf(requested) >= 0 ? requested : "en";

  if (typeof global.WBWWB_SET_LOCALE === "function") {
    global.WBWWB_SET_LOCALE(locale);
  }

  // An explicit URL choice wins over an old persisted preference.
  try {
    if (locale === "tr" && global.localStorage) {
      global.localStorage.setItem("wbwwb.locale", "tr");
    } else if (global.localStorage) {
      global.localStorage.setItem("wbwwb.locale", locale);
    }
  } catch (_) {}
})(window);
