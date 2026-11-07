# Woning HVAC

Ventilatie (en later meer) bediend met een ESP32-controller via een eigen webpagina.

| | |
|---|---|
| **Toestel** | Ventilatie-unit Begetube 350 R (0–10 V-sturing) |
| **Controller** | ESP32 Dev Module (CP2102, micro-USB) op het Zarlar-shield |
| **Omzetter** | AnaBoX v2.0 (PWM → 0–10 V), zonder R3 |
| **Sketch** | `ESP32_PWM_Test` v1.5: map `ESP32_PWM_Test/` met `ESP32_PWM_Test.ino` + `pagina.h` (rechtstreeks te openen in de Arduino IDE) |
| **Klaar om te uploaden** | `bin/ESP32_PWM_Test_v1.5.ino.bin` (board ESP32 Dev Module, standaard partities): klaar om te uploaden via `/update`, gecompileerd door Claude (ESP32-core 3.3.12, nieuwste bibliotheken). Alleen de laatste versie |
| **Status** | Stap 1: getest en werkend op de eigen Begetube (kelder, 1/11/2026); de unit van Maarten volgt |
| **Repo** | `FidelDworp/HVAC_WONING` |
| **Laatst bijgewerkt** | 3 november 2026 |

**Dit document heeft twee delen**
- **Deel A, voor Maarten:** wat is de bedoeling, in gewone woorden.
- **Deel B, technisch:** opstelling, metingen en volgende stappen.

---

# Deel A — Voor Maarten: wat is de bedoeling?

## A1. Het idee in het kort

Je ventilatie-unit heeft een ingang waarmee je de snelheid kiest met een spanning tussen **0 en 10 volt**. Normaal hangt daar een schakelaar of controller aan. Wij hangen er een klein computertje aan, zodat je de ventilatie met je **gsm** kunt regelen, en later eventueel **automatisch** (bijvoorbeeld harder bij douchen of koken).

```
 [ gsm / laptop ]  ── wifi ──>  [ ESP32 ]  ── signaal ──>  [ AnaBoX ]  ── 0–10 V ──>  [ ventilatie-unit ]
   schuifbalk                 klein computertje           omzetter                    draait sneller/trager
```

1. **De ESP32** is een computertje zo groot als een luciferdoosje, met wifi. Hij maakt een webpagina met een schuifbalk.
2. **Het signaal (PWM):** de ESP32 zet zijn uitgang heel snel aan en uit (duizenden keren per seconde). Hoe langer "aan", hoe hoger de stand.
3. **De AnaBoX** maakt van dat snelle aan/uit een rustige spanning van 0 tot 10 V voor de ventilatie-unit.

## A2. Is dit gevaarlijk of moeilijk?

**Nee.** Je werkt alleen met lage spanningen: 5 V (USB-lader) en 10 V. Dat is ongevaarlijk, zoals een batterij. De 230 V-kant van de ventilatie-unit blijft dicht.

Om te beginnen hoef je niets te solderen of te programmeren: een paar draadjes vastschroeven, de USB-lader erin en met de gsm aan de schuifbalk schuiven.

## A3. Wat kan er later bij? (enkel als je dat wil)

Dezelfde soort controller wordt al gebruikt voor veel meer dan ventilatie, bijvoorbeeld:

- **Verwarming:** een ketel of verwarmingscircuit sturen volgens de temperatuur.
- **Stopcontacten:** Shelly-stopcontacten aan/uit zetten.
- **Sensoren:** temperatuur, vochtigheid, licht, beweging, CO₂.

**Home Assistant op een Raspberry Pi** past daar goed bij. Het brengt al je slimme toestellen samen in één app, met automatisaties als "stijgt de vochtigheid in de badkamer, zet de ventilatie hoger". De ESP32 kan daar later aan gekoppeld worden (zie B4). Je kunt dus met de Raspberry Pi beginnen en de ESP32 later toevoegen, of omgekeerd.

---

# Deel B — Technisch

## B1. Opstelling

**Voeding:** één USB-lader (5 V, min. 1 A) op de micro-USB van de ESP32. De 5 V van de ESP32 gaat naar een **klein step-up-printje dat 10 V maakt** voor Vin van de AnaBoX. Zo is er geen aparte voeding nodig. Stel de 10 V in met de multimeter vóór je de AnaBoX aansluit. De uitgang van de AnaBoX schaalt mee met Vin.

| Van | Naar |
|---|---|
| GPIO 25 (schroefklem **P25** op het shield) | AnaBoX **PWM-IN** |
| GND (schroefklem links, tussen P26 en P33) | AnaBoX **GND** |
| 5 V van de ESP32 → step-up → **10 V** | AnaBoX **Vin** |
| AnaBoX **ANA-OUT** | Unit 0–10 V-ingang, **rood (+)** |
| AnaBoX **GND** | Unit **zwart (−)** |

⚠️ De klem **"CND"** op het shield is **geen GND** (drukfout voor CMD, GPIO 11). Daar niets op aansluiten, ook nooit op CMD/SD2/SD3 (GPIO 6–11).

## B2. Software en eerste gebruik

- **Arduino IDE** met **Arduino-ESP32 core 3.x**, board **"ESP32 Dev Module"** (niet ESP32-WROOM-DA: die gebruikt GPIO 25 als antenneschakelaar). Geen extra libraries.
- Beide bestanden in **dezelfde map** `ESP32_PWM_Test`. De webpagina staat bewust in `pagina.h`, anders breekt de Arduino IDE het JavaScript.
- Serial Monitor op **115200 baud** toont het IP-adres.

**Eerste keer:** verbind de gsm met het open wifi-netwerk **"PWM-Test"**. De instellingenpagina opent vanzelf (of `http://192.168.4.1`). Tik op **"Zoek netwerken"**, kies het thuisnetwerk, typ het wachtwoord, **"Verbind & bewaar"** (max. 5 netwerken). Daarna: **http://pwmtest.local**.

**De pagina's**

| Pagina | Inhoud |
|---|---|
| `/status` ("/" stuurt hierheen) | **Ventilatietest**: knoppen 3 / 25 / 50 / 75 / 100 %, groot blauw cijfer + schuifbalk 0-100 %, PWM-frequentie 1 / 5 / 20 kHz (standaard 5 kHz) |
| `/settings` | Netwerk en bewaarde netwerken · opstartstand (standaard 25 %, knop "Huidige stand als opstartstand bewaren") · kalibratie Vmax (9,3 V) en k (2) · systeem: firmware, vrij geheugen, on-line tijd, reden laatste herstart · knoppen **OTA update** en Herstarten |
| `/update` | Firmware uploaden met voortgangsbalk |

"100 %" geeft ≈ 9,3 V ≈ 93 %: meer haalt de AnaBoX niet met de unit aangesloten.

**OTA (sinds v1.5):**
1. De eerste keer v1.5 **via de kabel** uploaden; daarna kan het via wifi.
2. Arduino IDE → *Sketch → Export Compiled Binary*. In de map `build/esp32.esp32.esp32/` staan meerdere `.bin`-bestanden: kies **enkel `ESP32_PWM_Test.ino.bin`**, niet `.bootloader.bin`, `.partitions.bin` of `.merged.bin`.
3. `/settings` → **⬆ OTA update** (of rechtstreeks `/update`) → bestand kiezen → **Uploaden**. De balk toont de voortgang; na "✔ Gelukt" herstart de controller en na ±15 s opent `/status` weer.
4. Partition Scheme in de IDE: de standaard ("Default 4MB with spiffs", 1,2 MB per app) heeft plaats voor OTA. Meldt de IDE dat de sketch te groot is, kies dan "Minimal SPIFFS (1.9MB APP with OTA)" en flash één keer via de kabel. Een schema zonder OTA ("No OTA", "Huge APP") werkt niet.

**Onbereikbaar?** Is het netwerk **"PWM-Test"** zichtbaar, dan was de wifi weg (niet de controller). De sketch probeert elke 30 s opnieuw te verbinden; PWM-Test sluit 2 min na het herstel. `/settings` toont de reden van de laatste herstart.

⚠️ **0 V betekent volle kracht.** Onder 0,2 V draait de unit op 100 %. De sketch start daarom meteen in de opstartstand, ook zonder wifi, en de laagste stand ("0 %") blijft minstens 0,3 V. Een losgekoppelde draad of een uitgevallen controller geeft wel 100 %.

## B3. Metingen

**De ingang van de Begetube**
- **0,2–10 V = 0–100 %, lineair.** Onder 0,2 V (ook open) = 100 %.
- Ingangsweerstand ±**200 kΩ** (0,05 mA bij 10 V): een lichte belasting.

**Uitgang van de AnaBoX** (Vin = 10 V via step-up, multimeter)

| Preset | PWM | Onbelast | Met unit |
|---|---|---|---|
| 0 % | 0 | 0,00 V | 0,00 V |
| 25 % | 64 | 1,43 V | 1,36 V |
| 50 % | 128 | 3,34 V | 3,14 V |
| 75 % | 191 | 5,97 V | 5,53 V |
| 100 % | 255 | 9,97 V | niet gemeten |

**Ripple** (scoop, AC): 1 kHz: 0,18 V pp · 5 kHz: 0,03 V pp · 20 kHz: 0,01 V pp

**Conclusies**
- **De unit belast de AnaBoX maar licht** (±6 % lager). **R3 is niet nodig.**
- **De frequentie verandert de gelijkspanning niet.** De ripple daalt ongeveer met 1/f. **5 kHz** is de beste keuze: de ripple is dan ongeveer één PWM-stap (±0,04 V).
- **Het verloop is niet lineair maar wel regelmatig.** Het volgt `Vout = Vmax · D / (D + k·(1 − D))` met k ≈ 2 (D = duty cycle 0–1). Omgekeerd: `u = U / Vmax` en `D = k·u / (1 + (k − 1)·u)`. Met Vmax = 9,3 V voorspelt dat de metingen met unit tot op 0,04 V. Sinds v1.4.4 rekent de sketch zo om.

## B4. Koppeling met andere systemen (later)

| Adres | Doet |
|---|---|
| `/json` | Toestand als JSON (`v` = PWM 0–255, `u` = verwachte spanning, `p` = ventilatie-%, `vol` = volle kracht, `f` = frequentie, wifi, versie, `heap`/`heapp`/`blok` = geheugen, `up` = on-line (s), `rst` = reden laatste herstart) |
| `/setp?p=0…100` | Ventilatie-% zetten (met correctie, min. 0,3 V) |
| `/set?v=0…255` | Ruwe PWM-waarde zetten (0 = volle kracht!) |
| `/freq?f=1000\|5000\|20000` | Frequentie kiezen |
| `/cfg?vmax=…&k=…` | Kalibratie bewaren |
| `/savestart` | Huidige stand als opstartstand bewaren |
| `/reboot` | Herstarten |

In Home Assistant kan dat met een REST-sensor (uitlezen) en een REST-commando (zetten).

## B5. Volgende stappen

Werkwijze: telkens één stap, eerst akkoord van Filip.

- [x] Sketch **v1.4.4**: bediening in ventilatie-% met correctie, 5 kHz, instelbare opstartstand
- [x] v1.5: pagina's gesplitst (`/status`, `/settings`), OTA, wifi robuuster
- [ ] v1.5 via de kabel uploaden, daarna een OTA-test; spanning bij 100 % met de unit meten en als Vmax invullen (Kalibratie)
- [ ] Testen op de unit van Maarten
- [ ] Definitieve plaats van de controller (met goed wifi-bereik)
- [ ] *(optioneel, als Maarten wil)* Raspberry Pi met Home Assistant en koppeling (B4)

## B6. Versiegeschiedenis van de sketch

| Versie | Inhoud |
|---|---|
| v1.0 (21/10) | Schuifbalk 0–255, snelknoppen, frequentiekeuze, eigen wifi als terugval, mDNS |
| v1.1 (21/10) | Netwerkstatus op de pagina; board "ESP32 Dev Module" |
| v1.2 (21/10) | Wifi via de webpagina, max. 5 netwerken in NVS, knop "Vergeet" |
| v1.3 (21/10) | Captive portal: pagina opent vanzelf na verbinden met "PWM-Test" |
| v1.4–v1.4.1 (21/10) | Netwerken zoeken robuust gemaakt; compileerfout opgelost |
| v1.4.2 (22/10) | Webpagina naar `pagina.h` |
| v1.4.3 (22/10) | Bewaarde netwerken aantikken = meteen verbinden |
| v1.4.4 (1/11) | Bediening in ventilatie-% met correctie voor de AnaBoX-kromme, verwachte spanning op de pagina, opstartstand in NVS (standaard 25 %), kalibratie Vmax/k, standaard 5 kHz |
| v1.5 (3/11) | `/status` (enkel bediening) en `/settings` (netwerk, opstartstand, kalibratie, systeem); OTA met voortgangsbalk (`/update`); JSON naar `/json` met geheugen, on-line tijd en reden laatste herstart; wifi: geen modem-slaap, elke 30 s opnieuw verbinden, PWM-Test sluit na herstel |
