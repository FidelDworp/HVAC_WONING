// ============================================================================
// ESP32_PWM_Test.ino — v1.5 — 3 nov 2026
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
//   5 V van de ESP32 → step-up → 10 V        → AnaBoX Vin
//   AnaBoX ANA-OUT → unit rood (+); AnaBoX GND → unit zwart (−)
//
//   ⚠️ Klem "CND" op het shield is GEEN GND: drukfout voor CMD (GPIO 11, flash).
//      Nooit iets aansluiten op CMD/SD2/SD3 (GPIO 6-11).
//
// BEGETUBE-INGANG: 0,2-10 V = 0-100 % ventilatie (lineair). Onder 0,2 V (ook 0 V of open)
//   draait de unit op VOLLE KRACHT. PWM 0 betekent dus volle kracht!
//
// GEBRUIK:
//   - Pagina's: /status (bediening), /settings (netwerk, opstartstand, kalibratie, systeem,
//     OTA), /update (OTA met voortgangsbalk). "/" stuurt door naar /status (of /settings
//     zolang er geen WiFi is). JSON-data: /json.
//   - Eerste keer (of geen bekend netwerk in de buurt): verbind je gsm/Mac met het open
//     netwerk "PWM-Test" — /settings opent vanzelf (captive portal). Anders: http://192.168.4.1
//   - /settings → "Zoek netwerken" → netwerk aantikken → wachtwoord → "Verbind & bewaar".
//     Lukt het, dan wordt het netwerk in NVS bewaard (max. 5). Lukt het niet: niets bewaard.
//   - Bij het opstarten: eerst het laatst gebruikte bewaarde netwerk, dan de andere,
//     anders weer het eigen netwerk "PWM-Test".
//   - Met WiFi: IP op de Serial Monitor (115200 baud), of http://pwmtest.local
//   - Bij het opstarten (ook zonder wifi) gaat de PWM meteen naar de bewaarde opstartstand
//     (standaard 25 % ventilatie). Instellen: knop "Huidige stand als opstartstand bewaren".
//   - Bewaarde netwerken wissen: knop "Vergeet" op de webpagina.
//
// v1.5 (3nov26): - Webpagina gesplitst: /status (enkel bediening: titel "Ventilatietest", knoppen
//                   3/25/50/75/100 %, groot blauw cijfer + schuifbalk 0-100 %, PWM-frequentie),
//                   /settings (netwerk, bewaarde netwerken, opstartstand, kalibratie, systeem:
//                   firmware, vrij geheugen, on-line tijd, reden laatste herstart, OTA-knop,
//                   herstart) en /update (OTA met voortgangsbalk, Update.h).
//                 - JSON-endpoint /status → /json (+ heap, heapp, blok, up, rst).
//                 - WiFi robuuster na "onbereikbaar in de kelder" (3/11): WiFi.setSleep(false)
//                   (geen modem-slaap → webserver blijft antwoorden); zolang de verbinding
//                   weg is elke 30 s WiFi.reconnect(); komt ze vanzelf terug terwijl PWM-Test
//                   aan staat, dan sluit PWM-Test na 2 min.
//                 - /reboot.
// v1.4.4 (1nov26): bediening in ventilatie-% (0,2-10 V = 0-100 %) met correctie voor de
//                 niet-lineaire AnaBoX: U = Vmax·D/(D + k·(1−D)) ⇔ D = k·u/(1 + (k−1)·u),
//                 u = U/Vmax. Gemeten met de unit aangesloten: Vmax ≈ 9,3 V, k ≈ 2.
//                 Pagina toont verwachte spanning op ANA-OUT, ventilatie-% en PWM; waarschuwing
//                 onder 0,2 V (= volle kracht). Laagste stand ≥ 0,3 V (veiligheidsmarge).
//                 Opstartstand in NVS (namespace "pwmcfg", standaard 25 %) i.p.v. 0 V.
//                 Kalibratie (Vmax, k) instelbaar op de pagina. Standaardfrequentie 5 kHz
//                 (ripple 0,03 V i.p.v. 0,18 V bij 1 kHz). Nieuwe endpoints: /setp?p=0-100,
//                 /cfg?vmax=&k=, /savestart. /set?v=0-255 blijft (ruwe PWM).
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
#include <Update.h>        // v1.5: OTA via /update
#include <esp_system.h>    // v1.5: esp_reset_reason()

#define PWM_VERSION "1.5"

const char* AP_SSID   = "PWM-Test";           // eigen netwerk (open, zonder wachtwoord)
const char* MDNS_NAAM = "pwmtest";            // → http://pwmtest.local
const int   MAX_NET   = 5;                    // aantal bewaarde netwerken in NVS
const unsigned long VERBIND_MS = 15000;       // max. wachttijd per verbindingspoging
const unsigned long AP_NAZORG  = 120000;      // PWM-Test nog 2 min aan na geslaagde keuze

// ── PWM ─────────────────────────────────────────────────────────────────────
const int PWM_PIN  = 25;      // schroefklem P25
const int PWM_BITS = 8;       // 0-255
const uint32_t FREQS[] = { 1000, 5000, 20000 };
uint32_t pwm_freq  = 5000;    // 5 kHz: ripple ±0,03 V (1 kHz: 0,18 V)
int      pwm_value = 0;       // wordt in setup() meteen de opstartstand

// ── Begetube-ingang en AnaBoX-kromme ────────────────────────────────────────
const float U_MIN_UNIT   = 0.2;   // 0,2 V = 0 % ventilatie; lager = volle kracht!
const float U_MAX_UNIT   = 10.0;  // 10 V = 100 % ventilatie
const float U_MIN_VEILIG = 0.3;   // laagste stand via /setp blijft hierboven (marge)
const float START_P_STD  = 25.0;  // opstartstand zolang er niets bewaard is (%)
float cfg_vmax  = 9.3;            // ANA-OUT bij PWM 255, unit aangesloten (V)
float cfg_k     = 2.0;            // kromme AnaBoX: U = Vmax·D/(D + k·(1−D))
int   cfg_start = -1;             // bewaarde opstart-PWM (-1 = START_P_STD)

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
float  uVoorPwm(int v);
int    pwmVoorU(float U);
float  pVoorU(float U);
int    pwmVoorP(float p);
int    startPwm();
void   laadCfg();
void   bewaarCfg();
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

// ── Webpagina's: staan in pagina.h (zie v1.4.2 en v1.5) ─────────────────────
#include "pagina.h"

// ── Hulpfuncties ────────────────────────────────────────────────────────────
void zetPwm(int v) {
  pwm_value = constrain(v, 0, 255);
  ledcWrite(PWM_PIN, pwm_value);
  float U = uVoorPwm(pwm_value);
  if (U < U_MIN_UNIT)
    Serial.printf("[PWM] %d/255 ≈ %.2f V → unit op VOLLE KRACHT (< 0,2 V) @ %lu Hz\n", pwm_value, U, (unsigned long)pwm_freq);
  else
    Serial.printf("[PWM] %d/255 ≈ %.2f V ≈ %.0f %% ventilatie @ %lu Hz\n", pwm_value, U, pVoorU(U), (unsigned long)pwm_freq);
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

// ── Omrekenen PWM ↔ spanning ↔ ventilatie-% (zie v1.4.4) ───────────────────
float uVoorPwm(int v) {                      // verwachte spanning op ANA-OUT
  float D = v / 255.0f;
  if (D <= 0) return 0;
  return cfg_vmax * D / (D + cfg_k * (1 - D));
}

int pwmVoorU(float U) {                      // omgekeerde kromme
  if (U <= 0) return 0;
  float u = U / cfg_vmax;
  if (u >= 1) return 255;
  float D = cfg_k * u / (1 + (cfg_k - 1) * u);
  return constrain((int)lroundf(D * 255), 0, 255);
}

float pVoorU(float U) {                      // 0,2-10 V → 0-100 %
  return constrain((U - U_MIN_UNIT) / (U_MAX_UNIT - U_MIN_UNIT) * 100.0f, 0.0f, 100.0f);
}

int pwmVoorP(float p) {                      // 0-100 % → PWM, nooit onder U_MIN_VEILIG
  p = constrain(p, 0.0f, 100.0f);
  int v = pwmVoorU(U_MIN_UNIT + p / 100.0f * (U_MAX_UNIT - U_MIN_UNIT));
  while (v < 255 && uVoorPwm(v) < U_MIN_VEILIG) v++;
  return v;
}

int startPwm() { return cfg_start >= 0 ? cfg_start : pwmVoorP(START_P_STD); }

// Eigen namespace: bewaarNetwerken() wist "pwmtest" volledig (prefs.clear())
void laadCfg() {
  prefs.begin("pwmcfg", true);
  cfg_vmax  = prefs.getFloat("vmax", 9.3);
  cfg_k     = prefs.getFloat("k", 2.0);
  cfg_start = prefs.getInt("start", -1);
  prefs.end();
  if (!(cfg_vmax >= 5 && cfg_vmax <= 12)) cfg_vmax = 9.3;   // ook NaN opvangen
  if (!(cfg_k >= 0.5 && cfg_k <= 5))      cfg_k = 2.0;
  if (cfg_start > 255) cfg_start = -1;
}

void bewaarCfg() {
  prefs.begin("pwmcfg", false);
  prefs.putFloat("vmax", cfg_vmax);
  prefs.putFloat("k", cfg_k);
  prefs.putInt("start", cfg_start);
  prefs.end();
  Serial.printf("[CFG] Vmax %.2f V, k %.2f, opstart PWM %d\n", cfg_vmax, cfg_k, startPwm());
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
  WiFi.setSleep(false);                // v1.5: geen modem-slaap → webserver blijft bereikbaar
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
const char* resetReden() {               // v1.5
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:  return "stroom aan";
    case ESP_RST_SW:       return "software (OTA / herstart)";
    case ESP_RST_PANIC:    return "crash (PANIC)";
    case ESP_RST_INT_WDT:  return "watchdog (INT_WDT)";
    case ESP_RST_TASK_WDT: return "watchdog (TASK_WDT)";
    case ESP_RST_WDT:      return "watchdog (WDT)";
    case ESP_RST_BROWNOUT: return "spanningsdip (BROWNOUT)";
    case ESP_RST_EXT:      return "reset-knop";
    default:               return "onbekend";
  }
}

void stuurStatus() {
  float U  = uVoorPwm(pwm_value);
  float Us = uVoorPwm(startPwm());
  String j = "{\"v\":" + String(pwm_value) + ",\"f\":" + String(pwm_freq)
           + ",\"u\":" + String(U, 2) + ",\"p\":" + String(pVoorU(U), 1)
           + ",\"vol\":" + (U < U_MIN_UNIT ? "true" : "false")
           + ",\"pm\":" + String(pVoorU(cfg_vmax), 1)
           + ",\"vmax\":" + String(cfg_vmax, 2) + ",\"k\":" + String(cfg_k, 2)
           + ",\"st\":" + String(startPwm()) + ",\"stp\":" + String(pVoorU(Us), 1)
           + ",\"stvol\":" + (Us < U_MIN_UNIT ? "true" : "false")
           + ",\"sta\":" + (sta_ok ? "true" : "false")
           + ",\"ap\":" + (ap_aan ? "true" : "false")
           + ",\"ssid\":" + jsonTekst(sta_ok ? WiFi.SSID() : String(""))
           + ",\"ip\":\"" + (sta_ok ? WiFi.localIP().toString() : String("")) + "\""
           + ",\"rssi\":" + String(sta_ok ? (int)WiFi.RSSI() : 0)
           + ",\"bezig\":" + (bezig ? "true" : "false")
           + ",\"melding\":" + jsonTekst(melding)
           + ",\"bewaard\":[";
  for (int i = 0; i < net_aantal; i++) { if (i) j += ","; j += jsonTekst(net_ssid[i]); }
  j += "],\"ver\":\"" PWM_VERSION "\"";
  j += ",\"heap\":" + String(ESP.getFreeHeap() / 1024)
     + ",\"heapp\":" + String((uint32_t)((uint64_t)ESP.getFreeHeap() * 100 / ESP.getHeapSize()))
     + ",\"blok\":" + String(ESP.getMaxAllocHeap() / 1024)
     + ",\"up\":" + String(millis() / 1000)
     + ",\"rst\":" + jsonTekst(resetReden()) + "}";
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
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/settings", true);
  server.sendHeader("Cache-Control", "no-cache");
  server.send(302, "text/plain", "");
}

// ── Setup & loop ────────────────────────────────────────────────────────────
void setup() {
  // PWM meteen op de opstartstand — vóór WiFi, zodat de unit ook zonder wifi niet op
  // volle kracht blijft draaien (0 V = volle kracht bij de Begetube)
  laadCfg();
  pwm_value = startPwm();
  ledcAttach(PWM_PIN, pwm_freq, PWM_BITS);
  ledcWrite(PWM_PIN, pwm_value);

  Serial.begin(115200);
  delay(500);
  Serial.printf("\n\n=== ESP32 PWM-test v%s — GPIO %d ===\n", PWM_VERSION, PWM_PIN);
  Serial.printf("[PWM] opstart %d/255 ≈ %.2f V ≈ %.0f %% ventilatie @ %lu Hz (Vmax %.2f V, k %.2f)\n",
                pwm_value, uVoorPwm(pwm_value), pVoorU(uVoorPwm(pwm_value)), (unsigned long)pwm_freq,
                cfg_vmax, cfg_k);

  laadNetwerken();

  server.on("/", []() {                 // v1.5: doorsturen
    server.sendHeader("Location", (ap_aan && !sta_ok) ? "/settings" : "/status", true);
    server.send(302, "text/plain", "");
  });
  server.on("/status",   []() { server.send_P(200, "text/html; charset=utf-8", PAGINA_STATUS); });
  server.on("/settings", []() { server.send_P(200, "text/html; charset=utf-8", PAGINA_SETTINGS); });
  server.on("/update", HTTP_GET, []() { server.send_P(200, "text/html; charset=utf-8", PAGINA_OTA); });
  server.on("/update", HTTP_POST, []() {  // v1.5: OTA
    bool ok = !Update.hasError();
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", ok ? "OK" : Update.errorString());
    if (ok) { delay(500); ESP.restart(); }
  }, []() {
    HTTPUpload& up = server.upload();
    if (up.status == UPLOAD_FILE_START) {
      Serial.printf("[OTA] Start: %s\n", up.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
    } else if (up.status == UPLOAD_FILE_WRITE) {
      if (Update.write(up.buf, up.currentSize) != up.currentSize) Update.printError(Serial);
    } else if (up.status == UPLOAD_FILE_END) {
      if (Update.end(true)) Serial.printf("[OTA] OK: %u bytes — herstart\n", up.totalSize);
      else                  Update.printError(Serial);
    } else if (up.status == UPLOAD_FILE_ABORTED) {
      Update.abort();
      Serial.println("[OTA] Afgebroken");
    }
  });
  server.on("/reboot", []() {
    server.send(200, "text/plain", "Herstart…");
    delay(500);
    ESP.restart();
  });
  server.on("/set", []() {
    if (server.hasArg("v")) zetPwm(server.arg("v").toInt());
    stuurStatus();
  });
  server.on("/setp", []() {             // ventilatie-% (0-100), met correctie
    if (server.hasArg("p")) zetPwm(pwmVoorP(server.arg("p").toFloat()));
    stuurStatus();
  });
  server.on("/cfg", []() {              // kalibratie: Vmax (5-12 V) en k (0,5-5)
    if (server.hasArg("vmax")) { float x = server.arg("vmax").toFloat(); if (x >= 5 && x <= 12) cfg_vmax = x; }
    if (server.hasArg("k"))    { float x = server.arg("k").toFloat();    if (x >= 0.5 && x <= 5) cfg_k = x; }
    bewaarCfg();
    stuurStatus();
  });
  server.on("/savestart", []() {        // huidige stand = opstartstand
    cfg_start = pwm_value;
    bewaarCfg();
    stuurStatus();
  });
  server.on("/freq", []() {
    if (server.hasArg("f")) zetFreq((uint32_t)server.arg("f").toInt());
    stuurStatus();
  });
  server.on("/json", stuurStatus);      // v1.5: was /status
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
  static unsigned long laatste_poging = 0;
  if (sta_ok && !nu_ok && !bezig) {
    if (!kwijt_sinds) kwijt_sinds = millis();
    if (millis() - kwijt_sinds > 60000) { sta_ok = false; kwijt_sinds = 0; apAan(); Serial.println("[WiFi] Verbinding kwijt"); }
  } else {
    kwijt_sinds = 0;
    if (!sta_ok && nu_ok && !bezig) {     // vanzelf hersteld
      sta_ok = true;
      startMdns();
      if (ap_aan && !ap_sluit_om) ap_sluit_om = millis() + AP_NAZORG;   // v1.5: PWM-Test na 2 min dicht
      Serial.printf("[WiFi] Hersteld — http://%s\n", WiFi.localIP().toString().c_str());
    }
  }
  // v1.5: verbinding weg → elke 30 s opnieuw proberen
  // (enkel na een eerdere verbinding: anders stoort het zoeken het eigen netwerk PWM-Test)
  static bool ooit_verbonden = false;
  if (nu_ok) ooit_verbonden = true;
  if (!nu_ok && !bezig && ooit_verbonden && millis() - laatste_poging > 30000) {
    laatste_poging = millis();
    WiFi.reconnect();
  }

  delay(2);
}
