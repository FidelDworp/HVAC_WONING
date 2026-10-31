// ============================================================================
// ESP32_PWM_Test.ino — v1.4.3 — 22 okt 2026
// Eenvoudige PWM-testcontroller voor de AnaBoX v2.0 (ventilatie Begetube 350 R)
// Filip Delannoy — Zarlar thuisautomatisering
//
// Board : "ESP32 Dev Module" (CP2102, micro-USB)
// Core  : Arduino-ESP32 3.x (ledcAttach / ledcWrite / ledcChangeFrequency)
// Libs  : enkel ingebouwd (WiFi, WebServer, ESPmDNS, Preferences, DNSServer) — niets te installeren
//
// AANSLUITEN:
//   GPIO 25 (schroefklem P25)                → AnaBoX PWM-IN (signaal)
//   GND schroefklem links tussen P26 en P33  → AnaBoX GND
//   AnaBoX Vin = exact 10 V; GND AnaBoX = GND ventilatie-unit (klem 8)
//
//   ⚠️ Klem "CND" op het shield is GEEN GND: drukfout voor CMD (GPIO 11, flash).
//      Nooit iets aansluiten op CMD/SD2/SD3 (GPIO 6-11).
//
// GEBRUIK:
//   - Eerste keer (of geen bekend netwerk in de buurt): verbind je gsm/Mac met het open
//     netwerk "PWM-Test" — de pagina opent vanzelf (captive portal). Anders: http://192.168.4.1
//   - Webpagina → "Zoek netwerken" → netwerk aantikken → wachtwoord → "Verbind & bewaar".
//     Lukt het, dan wordt het netwerk in NVS bewaard (max. 5). Lukt het niet: niets bewaard.
//   - Bij het opstarten: eerst het laatst gebruikte bewaarde netwerk, dan de andere,
//     anders weer het eigen netwerk "PWM-Test".
//   - Met WiFi: IP op de Serial Monitor (115200 baud), of http://pwmtest.local
//   - Bij het opstarten staat PWM ALTIJD op 0 (ventilator slaat nooit vanzelf aan).
//   - Bewaarde netwerken wissen: knop "Vergeet" op de webpagina.
//
// v1.4.3 (22okt26): bewaarde netwerken echt bruikbaar: naam aantikken onder "Bewaarde
//                 netwerken" → meteen verbinden met het bewaarde wachtwoord (label
//                 "● verbonden" bij het huidige); in de zoeklijst "✓ bewaard" en verbinden
//                 zonder wachtwoordveld, met linkje "ander wachtwoord". Wachtwoordveld enkel
//                 nog bij een nieuw, beveiligd netwerk. /connect aanvaardt "bewaard=1".
// v1.4.2 (22okt26): webpagina naar apart bestand pagina.h. Oorzaak van "pagina doet niets":
//                 de Arduino IDE zette bij het compileren "#line …" en "function x();"
//                 midden in het JavaScript (ze zag de JS-functies aan voor C++-functies),
//                 waardoor Safari het hele script weigerde. .h-bestanden blijven ongemoeid.
//                 BESTANDEN: ESP32_PWM_Test.ino + pagina.h, samen in de map ESP32_PWM_Test.
// v1.4.1 (21okt26): compileerfout opgelost ("'doeScan' was not declared in this scope"):
//                 expliciete vooraankondigingen van alle eigen functies bovenaan;
//                 zoeklijst met vaste array (max. 40 netwerken) i.p.v. variabele lengte.
// v1.4 (21okt26): zoeken naar netwerken robuust gemaakt (zoals WiFiManager):
//                 - één keer zoeken bij het opstarten, vóór PWM-Test start (stoort niemand);
//                 - "Zoek netwerken" toont meteen die lijst en zoekt dan opnieuw (synchroon,
//                   ±3-5 s); mislukt dat, dan blijft de vorige lijst staan;
//                 - vóór het zoeken worden lopende verbindingspogingen gestopt;
//                 - de pagina meldt "geen antwoord" en probeert vanzelf opnieuw (niet meer stil);
//                 - meldingen op de Serial Monitor bij elke zoekopdracht.
// v1.3 (21okt26): captive portal op PWM-Test: kleine DNS-server stuurt elk adres naar
//                 192.168.4.1, en de testadressen van Apple (hotspot-detect.html), Android
//                 (generate_204) en Windows (connecttest/ncsi) leiden door naar de pagina →
//                 die opent vanzelf na het verbinden. Alleen actief zolang PWM-Test aan is.
// v1.2 (21okt26): WiFi volledig via de webpagina: lijst van ontvangen netwerken (zoeken op
//                 de achtergrond, gesorteerd op sterkte, slotje = wachtwoord nodig), kiezen +
//                 wachtwoord → pas na een geslaagde verbinding bewaard in NVS (namespace
//                 "pwmtest", max. 5 netwerken, laatst gebruikt eerst). Mislukt → terug naar
//                 het vorige netwerk. Knop "Vergeet". Wachtwoorden niet meer in de sketch en
//                 nooit terug naar de webpagina. Vanuit PWM-Test blijft dat netwerk nog
//                 2 min aan na een geslaagde verbinding, zodat je het resultaat ziet.
// v1.1 (21okt26): netwerkstatus op de UI (SSID, RSSI, balkjes + beoordeling) en netwerkkeuze
//                 uit een vaste lijst. Board-aanduiding "ESP32 Dev Module" (niet
//                 ESP32-WROOM-DA: die gebruikt GPIO 25 als antenneschakelaar).
// v1.0 (21okt26): eerste versie — slider 0-255, snelknoppen 0/25/50/75/100 %,
//                 frequentiekeuze 1 kHz / 5 kHz / 20 kHz, AP-fallback, mDNS.
// ============================================================================

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <DNSServer.h>

#define PWM_VERSION "1.4.3"

const char* AP_SSID   = "PWM-Test";           // eigen netwerk (open, zonder wachtwoord)
const char* MDNS_NAAM = "pwmtest";            // → http://pwmtest.local
const int   MAX_NET   = 5;                    // aantal bewaarde netwerken in NVS
const unsigned long VERBIND_MS = 15000;       // max. wachttijd per verbindingspoging
const unsigned long AP_NAZORG  = 120000;      // PWM-Test nog 2 min aan na geslaagde keuze

// ── PWM ─────────────────────────────────────────────────────────────────────
const int PWM_PIN  = 25;      // schroefklem P25
const int PWM_BITS = 8;       // 0-255
const uint32_t FREQS[] = { 1000, 5000, 20000 };
uint32_t pwm_freq  = 1000;    // start zoals de HVAC nu (1 kHz)
int      pwm_value = 0;       // start ALTIJD op 0

// ── Netwerk-toestand ────────────────────────────────────────────────────────
WebServer   server(80);
DNSServer   dns;                       // captive portal (alleen met PWM-Test)
Preferences prefs;
String net_ssid[MAX_NET], net_pass[MAX_NET];
int    net_aantal  = 0;
bool   ap_aan      = false;            // eigen netwerk PWM-Test actief
bool   sta_ok      = false;            // verbonden met een WiFi-netwerk
unsigned long ap_sluit_om = 0;         // > 0: PWM-Test sluiten op dit tijdstip
bool   verbind_vraag = false;          // in loop() een nieuwe verbinding proberen
String vraag_ssid, vraag_pass;
String melding = "";                   // laatste resultaat voor de webpagina
bool   bezig   = false;                // verbindingspoging loopt
String scan_lijst = "";                // laatste zoekresultaat (JSON-array zonder [ ])
unsigned long scan_om = 0;             // millis() van dat resultaat (0 = nog nooit)

// ── Vooraankondigingen (de Arduino IDE maakt ze niet altijd zelf) ───────────
void   zetPwm(int v);
void   zetFreq(uint32_t f);
String jsonTekst(const String& x);
void   laadNetwerken();
void   bewaarNetwerken();
void   onthoud(const String& ssid, const String& pass);
void   vergeet(int idx);
void   apAan();
void   apUit();
void   startMdns();
bool   probeer(const String& ssid, const String& pass);
void   startNetwerk();
void   verwerkVerbindVraag();
void   stuurStatus();
bool   doeScan();
void   stuurLijst(bool ok);
void   naarPortaal();

// ── Webpagina: staat in pagina.h (zie v1.4.2) ───────────────────────────────
#include "pagina.h"

// ── Hulpfuncties ────────────────────────────────────────────────────────────
void zetPwm(int v) {
  pwm_value = constrain(v, 0, 255);
  ledcWrite(PWM_PIN, pwm_value);
  Serial.printf("[PWM] %d/255 (%d %%) @ %lu Hz\n", pwm_value, pwm_value * 100 / 255, (unsigned long)pwm_freq);
}

void zetFreq(uint32_t f) {
  bool geldig = false;
  for (uint32_t g : FREQS) if (g == f) geldig = true;
  if (!geldig) return;
  pwm_freq = f;
  ledcChangeFrequency(PWM_PIN, pwm_freq, PWM_BITS);
  ledcWrite(PWM_PIN, pwm_value);   // waarde opnieuw zetten na frequentiewissel
  Serial.printf("[PWM] frequentie → %lu Hz\n", (unsigned long)pwm_freq);
}

String jsonTekst(const String& x) {          // aanhalingstekens en backslash ontsnappen
  String o = "\"";
  for (size_t i = 0; i < x.length(); i++) {
    char c = x[i];
    if (c == '"' || c == '\\') o += '\\';
    if ((uint8_t)c >= 0x20) o += c;
  }
  return o + "\"";
}

// ── NVS: bewaarde netwerken (index 0 = laatst gebruikt) ─────────────────────
void laadNetwerken() {
  prefs.begin("pwmtest", true);
  net_aantal = constrain(prefs.getInt("n", 0), 0, MAX_NET);
  for (int i = 0; i < net_aantal; i++) {
    char k[4];
    snprintf(k, sizeof(k), "s%d", i); net_ssid[i] = prefs.getString(k, "");
    snprintf(k, sizeof(k), "p%d", i); net_pass[i] = prefs.getString(k, "");
  }
  prefs.end();
  Serial.printf("[NVS] %d bewaarde netwerk(en)\n", net_aantal);
}

void bewaarNetwerken() {
  prefs.begin("pwmtest", false);
  prefs.clear();
  prefs.putInt("n", net_aantal);
  for (int i = 0; i < net_aantal; i++) {
    char k[4];
    snprintf(k, sizeof(k), "s%d", i); prefs.putString(k, net_ssid[i]);
    snprintf(k, sizeof(k), "p%d", i); prefs.putString(k, net_pass[i]);
  }
  prefs.end();
}

// Netwerk vooraan zetten (nieuw of bestaand); oudste valt weg bij > MAX_NET
void onthoud(const String& ssid, const String& pass) {
  int bestaand = -1;
  for (int i = 0; i < net_aantal; i++) if (net_ssid[i] == ssid) bestaand = i;
  int laatste = (bestaand >= 0) ? bestaand : min(net_aantal, MAX_NET - 1);
  for (int i = laatste; i > 0; i--) { net_ssid[i] = net_ssid[i - 1]; net_pass[i] = net_pass[i - 1]; }
  net_ssid[0] = ssid; net_pass[0] = pass;
  if (bestaand < 0 && net_aantal < MAX_NET) net_aantal++;
  bewaarNetwerken();
  Serial.printf("[NVS] '%s' bewaard (%d netwerk(en))\n", ssid.c_str(), net_aantal);
}

void vergeet(int idx) {
  if (idx < 0 || idx >= net_aantal) return;
  Serial.printf("[NVS] '%s' vergeten\n", net_ssid[idx].c_str());
  for (int i = idx; i < net_aantal - 1; i++) { net_ssid[i] = net_ssid[i + 1]; net_pass[i] = net_pass[i + 1]; }
  net_aantal--;
  bewaarNetwerken();
}

// ── WiFi ────────────────────────────────────────────────────────────────────
void apAan() {
  if (ap_aan) return;
  WiFi.mode(WIFI_AP_STA);                          // STA blijft beschikbaar voor zoeken/verbinden
  WiFi.softAP(AP_SSID);                            // open, zonder wachtwoord
  dns.start(53, "*", WiFi.softAPIP());             // captive portal: elk adres → 192.168.4.1
  ap_aan = true;
  Serial.printf("[WiFi] Eigen netwerk '%s' → http://%s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
}

void apUit() {
  if (!ap_aan) return;
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  ap_aan = false;
  Serial.println("[WiFi] Eigen netwerk PWM-Test gesloten");
}

void startMdns() {
  MDNS.end();
  if (MDNS.begin(MDNS_NAAM)) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("[mDNS] http://%s.local\n", MDNS_NAAM);
  }
}

// Eén poging; houdt de webserver bereikbaar via PWM-Test als dat actief is
bool probeer(const String& ssid, const String& pass) {
  Serial.printf("[WiFi] Verbinden met '%s'", ssid.c_str());
  WiFi.disconnect(false, true);        // STA los, radio blijft aan
  delay(200);
  if (pass.length()) WiFi.begin(ssid.c_str(), pass.c_str());
  else               WiFi.begin(ssid.c_str());
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < VERBIND_MS) {
    delay(250);
    if (ap_aan) { dns.processNextRequest(); server.handleClient(); }  // pagina blijft antwoorden via PWM-Test
    if ((millis() - start) % 1000 < 250) Serial.print(".");
  }
  sta_ok = (WiFi.status() == WL_CONNECTED);
  if (sta_ok) Serial.printf(" OK — http://%s (RSSI %d dBm)\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
  else        Serial.println(" mislukt");
  return sta_ok;
}

// Opstart: bewaarde netwerken (laatst gebruikt eerst), anders PWM-Test
void startNetwerk() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  for (int i = 0; i < net_aantal; i++) {
    if (probeer(net_ssid[i], net_pass[i])) {
      if (i > 0) onthoud(net_ssid[i], net_pass[i]);   // wordt "laatst gebruikt"
      startMdns();
      return;
    }
  }
  WiFi.disconnect(false, true);
  doeScan();                           // lijst klaarzetten vóór PWM-Test start (stoort niemand)
  apAan();
}

// Gevraagd via de webpagina (uitgevoerd in loop())
void verwerkVerbindVraag() {
  String oud_ssid = sta_ok ? WiFi.SSID() : "";
  String oud_pass = "";
  for (int i = 0; i < net_aantal; i++) if (net_ssid[i] == oud_ssid) oud_pass = net_pass[i];

  bezig = true;
  if (probeer(vraag_ssid, vraag_pass)) {
    onthoud(vraag_ssid, vraag_pass);
    startMdns();
    melding = "✔ Verbonden met " + vraag_ssid + " en bewaard — http://" + WiFi.localIP().toString() + " of http://pwmtest.local";
    if (ap_aan) ap_sluit_om = millis() + AP_NAZORG;   // nog 2 min bereikbaar via PWM-Test
  } else {
    melding = "✘ Verbinden met " + vraag_ssid + " mislukt (wachtwoord fout of netwerk te zwak) — niets bewaard";
    if (oud_ssid.length() && probeer(oud_ssid, oud_pass)) {
      startMdns();
      melding += ". Terug op " + oud_ssid + ".";
    } else {
      WiFi.disconnect(false, true);
      apAan();
      melding += ". Eigen netwerk PWM-Test actief (http://192.168.4.1).";
    }
  }
  vraag_pass = "";                     // wachtwoord niet langer dan nodig in RAM
  bezig = false;
  Serial.println("[WiFi] " + melding);
}

// ── Webserver ───────────────────────────────────────────────────────────────
void stuurStatus() {
  String j = "{\"v\":" + String(pwm_value) + ",\"f\":" + String(pwm_freq)
           + ",\"sta\":" + (sta_ok ? "true" : "false")
           + ",\"ap\":" + (ap_aan ? "true" : "false")
           + ",\"ssid\":" + jsonTekst(sta_ok ? WiFi.SSID() : String(""))
           + ",\"ip\":\"" + (sta_ok ? WiFi.localIP().toString() : String("")) + "\""
           + ",\"rssi\":" + String(sta_ok ? (int)WiFi.RSSI() : 0)
           + ",\"bezig\":" + (bezig ? "true" : "false")
           + ",\"melding\":" + jsonTekst(melding)
           + ",\"bewaard\":[";
  for (int i = 0; i < net_aantal; i++) { if (i) j += ","; j += jsonTekst(net_ssid[i]); }
  j += "],\"ver\":\"" PWM_VERSION "\"}";
  server.send(200, "application/json", j);
}

// Zoeken naar netwerken (synchroon). Resultaat → scan_lijst. Mislukt → vorige lijst blijft.
bool doeScan() {
  if (!sta_ok && !bezig) WiFi.disconnect(false, false);   // lopende verbindingspoging stoppen
  if (WiFi.getMode() == WIFI_AP) WiFi.mode(WIFI_AP_STA);   // zoeken vraagt de STA-kant
  Serial.println("[SCAN] Zoeken naar netwerken…");
  unsigned long t0 = millis();
  int n = WiFi.scanNetworks(false, false);                  // blokkeert ±2-5 s
  if (n < 0) {
    Serial.printf("[SCAN] Mislukt (code %d) — vorige lijst blijft\n", n);
    WiFi.scanDelete();
    return false;
  }
  // Indexen sorteren op sterkte; dubbele namen en verborgen netwerken overslaan
  const int MAX_SCAN = 40;                     // vaste array: geen geheugenverrassingen
  int m = n < MAX_SCAN ? n : MAX_SCAN;
  int idx[MAX_SCAN];
  for (int i = 0; i < m; i++) idx[i] = i;
  for (int i = 0; i < m; i++) for (int k = i + 1; k < m; k++)
    if (WiFi.RSSI(idx[k]) > WiFi.RSSI(idx[i])) { int x = idx[i]; idx[i] = idx[k]; idx[k] = x; }
  String j = "";
  int getoond = 0;
  for (int a = 0; a < m; a++) {
    String naam = WiFi.SSID(idx[a]);
    if (!naam.length()) continue;
    bool dubbel = false;
    for (int b = 0; b < a; b++) if (WiFi.SSID(idx[b]) == naam) dubbel = true;
    if (dubbel) continue;
    if (getoond++) j += ",";
    j += "{\"s\":" + jsonTekst(naam) + ",\"r\":" + String(WiFi.RSSI(idx[a]))
       + ",\"o\":" + String(WiFi.encryptionType(idx[a]) == WIFI_AUTH_OPEN ? 1 : 0) + "}";
  }
  WiFi.scanDelete();
  scan_lijst = j;
  scan_om = millis();
  Serial.printf("[SCAN] %d netwerk(en) gevonden, %d getoond (%lu ms)\n", n, getoond, millis() - t0);
  return true;
}

void stuurLijst(bool ok) {
  unsigned long leeftijd = scan_om ? (millis() - scan_om) / 1000 : 0;
  server.send(200, "application/json",
    "{\"ok\":" + String(ok ? "true" : "false") + ",\"leeftijd\":" + String(leeftijd)
    + ",\"netten\":[" + scan_lijst + "]}");
}

// Captive portal: doorsturen naar de pagina op 192.168.4.1 (alleen met PWM-Test)
void naarPortaal() {
  if (!ap_aan) { server.send(404, "text/plain", "Niet gevonden"); return; }
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
  server.sendHeader("Cache-Control", "no-cache");
  server.send(302, "text/plain", "");
}

// ── Setup & loop ────────────────────────────────────────────────────────────
void setup() {
  // PWM eerst op 0 — vóór WiFi, zodat de ventilator nooit vanzelf aanslaat
  ledcAttach(PWM_PIN, pwm_freq, PWM_BITS);
  ledcWrite(PWM_PIN, 0);

  Serial.begin(115200);
  delay(500);
  Serial.printf("\n\n=== ESP32 PWM-test v%s — GPIO %d ===\n", PWM_VERSION, PWM_PIN);

  laadNetwerken();

  server.on("/", []() { server.send_P(200, "text/html; charset=utf-8", PAGINA); });
  server.on("/set", []() {
    if (server.hasArg("v")) zetPwm(server.arg("v").toInt());
    stuurStatus();
  });
  server.on("/freq", []() {
    if (server.hasArg("f")) zetFreq((uint32_t)server.arg("f").toInt());
    stuurStatus();
  });
  server.on("/status", stuurStatus);
  server.on("/scan", []() {             // nieuwe zoekopdracht; antwoord = de lijst
    bool ok = bezig ? false : doeScan(); // niet zoeken tijdens verbinden
    stuurLijst(ok);
  });
  server.on("/scanres", []() { stuurLijst(true); });   // laatste lijst, meteen
  server.on("/connect", HTTP_POST, []() {
    if (!server.hasArg("ssid") || !server.arg("ssid").length() || bezig) {
      server.send(400, "text/plain", "Geen netwerk gekozen of al bezig"); return;
    }
    vraag_ssid = server.arg("ssid");
    vraag_pass = server.hasArg("pass") ? server.arg("pass") : "";
    if (server.hasArg("bewaard")) {      // bewaard netwerk: wachtwoord uit NVS nemen
      int gevonden = -1;
      for (int i = 0; i < net_aantal; i++) if (net_ssid[i] == vraag_ssid) gevonden = i;
      if (gevonden < 0) { server.send(400, "text/plain", "Netwerk niet bewaard"); return; }
      vraag_pass = net_pass[gevonden];
    }
    verbind_vraag = true;                // pas in loop() verbinden, na het antwoord
    melding = "";
    server.send(200, "text/plain", "OK");
  });
  server.on("/forget", []() {
    if (server.hasArg("i")) vergeet(server.arg("i").toInt());
    stuurStatus();
  });
  // Captive portal: testadressen van Apple / Android / Windows → naar de pagina
  const char* testadressen[] = { "/hotspot-detect.html", "/library/test/success.html", "/generate_204",
                                 "/gen_204", "/connecttest.txt", "/ncsi.txt", "/fwlink", "/redirect" };
  for (const char* a : testadressen) server.on(a, naarPortaal);
  server.onNotFound([]() {
    if (ap_aan && server.hostHeader() != WiFi.softAPIP().toString()) { naarPortaal(); return; }
    server.send(404, "text/plain", "Niet gevonden");
  });

  startNetwerk();
  server.begin();
  Serial.println("[WEB] Klaar");
}

void loop() {
  if (ap_aan) dns.processNextRequest();  // captive portal
  server.handleClient();

  if (verbind_vraag) {                   // nieuwe verbinding gevraagd via de webpagina
    verbind_vraag = false;
    delay(300);                          // antwoord eerst laten vertrekken
    verwerkVerbindVraag();               // PWM blijft ongewijzigd lopen
  }

  if (ap_sluit_om && millis() > ap_sluit_om) {   // na geslaagde keuze PWM-Test sluiten
    ap_sluit_om = 0;
    if (sta_ok) apUit();
  }

  // Verbinding later verloren (bv. router herstart)? autoReconnect probeert opnieuw;
  // lukt dat 60 s niet, dan PWM-Test openen zodat het bord bereikbaar blijft.
  static unsigned long kwijt_sinds = 0;
  bool nu_ok = (WiFi.status() == WL_CONNECTED);
  if (sta_ok && !nu_ok && !bezig) {
    if (!kwijt_sinds) kwijt_sinds = millis();
    if (millis() - kwijt_sinds > 60000) { sta_ok = false; kwijt_sinds = 0; apAan(); Serial.println("[WiFi] Verbinding kwijt"); }
  } else {
    kwijt_sinds = 0;
    if (!sta_ok && nu_ok && !bezig) { sta_ok = true; startMdns(); }   // vanzelf hersteld
  }

  delay(2);
}
