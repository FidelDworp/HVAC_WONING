# Woning HVAC

Ventilatie (en later meer) in de woning van Maarten, bediend met een ESP32-controller via een eigen webpagina.

| | |
|---|---|
| **Toestel** | Ventilatie-unit Begetube 350 R (0–10 V-sturing) |
| **Controller** | ESP32 Dev Module (CP2102, micro-USB) op het Zarlar-shield |
| **Omzetter** | AnaBoX v2.0 (PWM → 0–10 V), standaard: zonder R3 (hoogohmige uitgang) |
| **Sketch** | `ESP32_PWM_Test` v1.4.3 (`ESP32_PWM_Test.ino` + `pagina.h`, samen in de map `ESP32_PWM_Test`) |
| **Status** | Stap 1: testopstelling, metingen zonder unit gedaan (31/10/2026), meting mét unit volgt |
| **Laatst bijgewerkt** | 31 oktober 2026 |

**Dit document heeft twee delen**
- **Deel A, voor Maarten:** wat is de bedoeling, in gewone woorden.
- **Deel B, technisch:** de testopstelling, de metingen en de volgende stappen.

---

# Deel A — Voor Maarten: wat is de bedoeling?

## A1. Het idee in het kort

Je ventilatie-unit heeft een ingang waarmee je de snelheid kiest met een spanning tussen **0 en 10 volt**: 0 V = (bijna) stil, 10 V = volle kracht. Normaal hangt daar een draaiknop of schakelaar aan. Wij hangen er een klein computertje aan, zodat je de ventilatie met je **gsm** kunt regelen, en later eventueel **automatisch** (bijvoorbeeld harder bij douchen of koken).

Er zijn drie blokjes:

```
 [ gsm / laptop ]  ── wifi ──>  [ ESP32 ]  ── signaal ──>  [ AnaBoX ]  ── 0–10 V ──>  [ ventilatie-unit ]
   schuifbalk                 klein computertje           omzetter                    draait sneller/trager
```

1. **De ESP32** is een computertje zo groot als een luciferdoosje, met wifi. Hij maakt een webpagina met een schuifbalk.
2. **Het signaal (PWM)**: de ESP32 kan geen 0–10 V maken. Hij zet zijn uitgang heel snel aan en uit (duizenden keren per seconde). Hoe langer "aan", hoe hoger de stand. Vergelijk het met een lichtschakelaar die je razendsnel aan en uit zet: hoe langer aan, hoe feller het lijkt.
3. **De AnaBoX** maakt van dat snelle aan/uit een rustige, vaste spanning van 0 tot 10 V. Die gaat naar de ventilatie-unit.

## A2. Is dit gevaarlijk of moeilijk?

**Nee.** In deze opstelling werk je alleen met lage spanningen: 5 V (USB-lader) en 10 V. Dat is ongevaarlijk, zoals een batterij. De 230 V-kant van de ventilatie-unit blijft dicht en daar hoef je niet aan te komen.

Je hoeft ook niets te solderen of te programmeren om te beginnen. Stap 1 is: een paar draadjes vastschroeven, de stekker erin en met de gsm aan de schuifbalk schuiven. De rest is optioneel.

## A3. Wat kan er later bij? (enkel als je dat wil)

Dezelfde soort controller wordt bij papa al gebruikt voor veel meer dan ventilatie. Een paar voorbeelden, niets daarvan is verplicht:

- **Verwarming:** een ketel of verwarmingscircuit aan/uit zetten volgens de temperatuur (zoals in de Sjalay).
- **Stopcontacten:** Shelly-stopcontacten aan/uit zetten (lampen, toestellen).
- **Sensoren:** temperatuur, vochtigheid, licht, beweging, CO₂.
- **Logboek:** metingen automatisch bijhouden in een Google-spreadsheet.

**Home Assistant op een Raspberry Pi** past daar goed bij. Home Assistant is een app (een "centrale") die al je slimme toestellen in één overzicht samenbrengt, met automatisaties als "als de vochtigheid in de badkamer stijgt, zet de ventilatie hoger". De ESP32 heeft een eenvoudige webverbinding (zie B7) waarmee Home Assistant hem kan uitlezen en bedienen. Je kunt dus perfect met de Raspberry Pi en Home Assistant beginnen en de ESP32 later toevoegen, of omgekeerd.

---

# Deel B — Technisch

## B1. Stap 1: de testopstelling

**Doel:** nagaan of de ESP32 + AnaBoX de ventilatie-unit netjes en voorspelbaar kan sturen, en meten hoe de uitgangsspanning zich gedraagt.

**Benodigdheden**
- ESP32 Dev Module op het shield + micro-USB-kabel + USB-lader (5 V)
- AnaBoX v2.0
- Een **10 V-voeding** voor de AnaBoX (zie B2)
- Multimeter (DC-spanning) en eventueel de Joy-IT-scoop
- Een paar draadjes, kleine platte schroevendraaier

**Aansluiten**

| Van | Naar |
|---|---|
| GPIO 25 (schroefklem **P25** op het shield) | AnaBoX **PWM-IN** (signaal) |
| GND (schroefklem links, tussen P26 en P33) | AnaBoX **GND** |
| 10 V-voeding **+** | AnaBoX **Vin** |
| 10 V-voeding **−** | AnaBoX **GND** |
| AnaBoX **GND** | GND van de ventilatie-unit (**klem 8**) |
| AnaBoX **ANA-OUT** | 0–10 V-ingang van de ventilatie-unit (zie handleiding van de unit) |

⚠️ **Let op**
- De klem **"CND"** op het shield is **geen GND**: het is een drukfout voor CMD (GPIO 11, flashgeheugen). Daar niets op aansluiten. Ook nooit iets op CMD/SD2/SD3 (GPIO 6–11).
- Alle GND's (ESP32, AnaBoX, 10 V-voeding, unit) moeten verbonden zijn, anders klopt de spanning niet.
- Eerst alles bedraden, dan pas de voedingen aanzetten.

## B2. Welke 10 V-voeding?

De uitgang van de AnaBoX kan nooit hoger gaan dan de spanning op Vin. Met 10 V op Vin is 100 % ≈ 10 V. Praktische keuzes, van handigst naar meest definitief:

1. **Labvoeding (bv. de DPS150)** op **10,00 V** met een stroomlimiet van **±100 mA**. Ideaal voor de test: exact instelbaar en beveiligd.
2. **Een 10 V-uitgang op de unit zelf.** Veel toestellen met een 0–10 V-ingang hebben op dezelfde klemmenstrook een +10 V-uitgang (bedoeld voor een draaiknop/potentiometer). Kijk in de handleiding van de Begetube 350 R. Is die er, dan is dat de eenvoudigste definitieve oplossing (en de GND is dan automatisch gedeeld).
3. **Een 12 V-adapter + klein regelbaar step-down-printje** (buck), ingesteld op 10,0 V met de multimeter vóór je de AnaBoX aansluit.

Wat je ook kiest: **eerst de spanning meten** voor je de AnaBoX aansluit.

## B3. Software installeren

- **Arduino IDE** met **Arduino-ESP32 core 3.x**
- Board: **"ESP32 Dev Module"** (niet ESP32-WROOM-DA: die gebruikt GPIO 25 als antenneschakelaar)
- Libraries: enkel ingebouwde, niets te installeren
- Beide bestanden (`ESP32_PWM_Test.ino` en `pagina.h`) in **dezelfde map** `ESP32_PWM_Test`. De webpagina staat bewust in `pagina.h`: anders zet de Arduino IDE stukjes C++ midden in het JavaScript en doet de pagina niets meer.
- Uploaden via USB; Serial Monitor op **115200 baud** toont het IP-adres.

## B4. Eerste gebruik

1. Zet de ESP32 aan. **De PWM staat bij het opstarten altijd op 0**: de ventilator slaat nooit vanzelf aan.
2. Eerste keer: verbind je gsm met het open wifi-netwerk **"PWM-Test"**. De pagina opent vanzelf (lukt dat niet: `http://192.168.4.1`).
3. Tik op **"Zoek netwerken"**, kies je thuisnetwerk, typ het wachtwoord, **"Verbind & bewaar"**. Max. 5 netwerken worden bewaard.
4. Daarna: gsm terug op je thuisnetwerk en open **http://pwmtest.local** (of het IP uit de Serial Monitor).

**Op de pagina**
- **Schuifbalk 0–255** en snelknoppen **0 / 25 / 50 / 75 / 100 %**, met de verwachte spanning (bij Vin = 10 V)
- **PWM-frequentie:** 1 kHz / 5 kHz / 20 kHz
- **Netwerk:** naam, signaalsterkte, bewaarde netwerken (aantikken = verbinden, "Vergeet" = wissen)

## B5. Metingen zonder unit (31/10/2026)

Gemeten op ANA-OUT met de Joy-IT-scoop (1 MΩ, dus vrijwel onbelast), AnaBoX zonder R3, Vin = 10 V.

**Gelijkspanning** (identiek bij 1, 5 en 20 kHz)

| Preset | PWM | Lineair verwacht | Gemeten |
|---|---|---|---|
| 0 % | 0 | 0,00 V | 0,00 V |
| 25 % | 64 | 2,51 V | 1,40 V |
| 50 % | 128 | 5,02 V | 3,25 V |
| 75 % | 191 | 7,49 V | 5,80 V |
| 100 % | 255 | 10,0 V | 9,68 V |

**Ripple** (AC-stand)

| Frequentie | Ripple (pp) |
|---|---|
| 1 kHz | 0,18 V |
| 5 kHz | 0,03 V |
| 20 kHz | 0,01 V (rand van wat de scoop meet) |

**Conclusies**
- **De frequentie verandert de gelijkspanning niet.** De filter van de AnaBoX strijkt elke frequentie goed glad.
- **Het verloop is niet lineair, maar wel regelmatig.** Alle punten passen op `Vout = 9,68 · D / (D + 2·(1 − D))` (D = duty cycle 0–1). Dat wijst op een filtercondensator die ±2× trager oplaadt dan ontlaadt (ongelijke weerstanden). Dat is een vermoeden: het schema van de AnaBoX is niet nagekeken.
- **Correctie in software is eenvoudig:** voor een gewenste spanning U is `u = U / 9,68` en `D = 2u / (1 + u)`. Echte 25 / 50 / 75 % worden dan PWM **102 / 170 / 219** (in plaats van 64 / 128 / 191).
- **Maximum 9,68 V** (± 97 %): ±0,3 V gaat verloren in de uitgangstrap. Voor een 0–10 V-ingang meestal geen probleem.
- **Ripple daalt ongeveer met 1/f** (gewone RC-filter). Eén PWM-stap is ±0,04 V. Bij 1 kHz is de ripple 4–5 stappen groot; bij 5 kHz ongeveer één stap. **Advies: 5 kHz als standaard.**

## B6. Stap 1b: meten mét de unit aangesloten (nog te doen)

De uitgang van de AnaBoX is hoogohmig (zonder R3). De scoop belast hem nauwelijks, maar de 0–10 V-ingang van de unit wel (vaak 10–100 kΩ). De spanningen en de kromme kunnen dus veranderen zodra de unit aangesloten is. Daarom opnieuw meten, met de **multimeter op DC** (geeft het gemiddelde):

| PWM | ANA-OUT met unit (V) | Gedrag ventilator (start? geluid? snelheid?) |
|---|---|---|
| 0 | | |
| 26 (10 %) | | |
| 64 (25 %) | | |
| 128 (50 %) | | |
| 191 (75 %) | | |
| 230 (90 %) | | |
| 255 (100 %) | | |

Ook:
- Ripple bij **50 %** (daar is ze het grootst), bij 1 kHz en 5 kHz
- Vanaf welke spanning begint de ventilator te draaien, en is er hoorbaar verschil tussen de frequenties?
- Zakt de spanning veel ten opzichte van B5? Dan beslissen of **R3** (lagere uitgangsimpedantie) toch geplaatst moet worden.

## B7. Koppeling met andere systemen (later)

De sketch heeft een eenvoudige webverbinding (HTTP), handig voor Home Assistant of de Zarlar-controllers:

| Adres | Doet |
|---|---|
| `/status` | Huidige toestand als JSON (`v` = PWM 0–255, `f` = frequentie, wifi-gegevens, versie) |
| `/set?v=0…255` | PWM-waarde zetten |
| `/freq?f=1000\|5000\|20000` | Frequentie kiezen |

In Home Assistant kan dat bijvoorbeeld met een REST-sensor (uitlezen) en een REST-commando (zetten). Dat werken we pas uit als het zover is.

## B8. Volgende stappen

Werkwijze: telkens één stap, eerst akkoord van Filip.

- [ ] 10 V-voeding kiezen (B2) en meten met de unit aangesloten (B6), samen met Maarten
- [ ] Beslissen over R3 op de AnaBoX op basis van B6
- [ ] *(na akkoord)* Sketch **v1.4.4**: schuifbalk en knoppen in **volt (0–10 V)** met de correctie `D = 2u / (1 + u)`, de factor "2" en het maximum (9,68 V) instelbaar na de meting met de unit, **5 kHz** als standaard bij het opstarten
- [ ] Definitieve plaats en voeding voor de controller in de woning
- [ ] *(optioneel, als Maarten wil)* Raspberry Pi met Home Assistant, en de ventilatie daaraan koppelen (B7)
- [ ] *(optioneel)* Uitbreiden: sensoren (vocht in de badkamer), Shelly-stopcontacten, verwarming

## B9. Versiegeschiedenis van de sketch

| Versie | Inhoud |
|---|---|
| v1.0 (21/10) | Schuifbalk 0–255, snelknoppen, frequentiekeuze 1/5/20 kHz, eigen wifi als terugval, mDNS |
| v1.1 (21/10) | Netwerkstatus op de pagina (naam, sterkte); board "ESP32 Dev Module" |
| v1.2 (21/10) | Wifi volledig via de webpagina, max. 5 netwerken bewaard in NVS, knop "Vergeet" |
| v1.3 (21/10) | Captive portal: de pagina opent vanzelf na verbinden met "PWM-Test" |
| v1.4 (21/10) | Netwerken zoeken robuust gemaakt |
| v1.4.1 (21/10) | Compileerfout opgelost (vooraankondigingen), vaste zoeklijst (max. 40) |
| v1.4.2 (22/10) | Webpagina naar `pagina.h` (de IDE brak het JavaScript) |
| v1.4.3 (22/10) | Bewaarde netwerken aantikken = meteen verbinden; wachtwoordveld enkel bij nieuwe netwerken |
