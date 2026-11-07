# Overdracht — Ventilatie woning Maarten (HVAC_WONING)

*Stand 7 november 2026 — Filip Delannoy (FiDel), Zarlardinge (BE)*

Dit document start een nieuw gesprek met Claude over de ventilatiecontroller voor de woning van Maarten: een ESP32 die de Begetube 350 R via een AnaBoX (PWM → 0–10 V) aanstuurt. Alle details: [`README.md`](../README.md) (deel A voor Maarten, deel B technisch). Uitleg voor beginners: `Zarlardinge-Home-Automation/docs/UITLEG_VOOR_MAARTEN.md`.

**Werkwijze en afspraken:** zie de [overkoepelende overdracht](https://github.com/FidelDworp/Zarlardinge-Home-Automation/blob/main/docs/ZARLAR_OVERDRACHT.md). Kort: Nederlands · nooit een sketch of script zonder GO · eerst een plan, één stap per versie (versiekop + changelog) en daarna een 48 u-test · alles via OTA · `/json`-keys en bedieningsadressen niet wijzigen zonder akkoord · README en deze overdracht mee bijwerken · na GO compileert Claude en zet de `.bin` in `bin/`.

---

## 1. Wat en waar

| | |
|---|---|
| Controller | **klassieke ESP32** Dev Module (CP2102, micro-USB) op het Zarlar-shield, `http://pwmtest.local` |
| Sketch | `ESP32_PWM_Test/` (`ESP32_PWM_Test.ino` + `pagina.h`) **v1.5** |
| Klaar om te uploaden | `bin/ESP32_PWM_Test_v1.5.ino.bin` (board ESP32 Dev Module, standaard partities "Default 4MB with spiffs") |
| Toestel | Begetube 350 R, 0–10 V-ingang. **Let op: 0 V = volle kracht**; de sketch start veilig en gaat nooit onder 0,3 V |

## 2. Stand (7/11)

- Stap 1 getest en werkend op de eigen Begetube (kelder, 1/11/2026). De unit van Maarten volgt.
- Compileert in de sessie met core 3.3.12 (1,02 MB, 77 % van de 1,25 MB-app-partitie).

## 3. Open punten

1. v1.5 via de kabel uploaden, daarna een OTA-test; spanning bij 100 % meten en als Vmax invullen.
2. Testen op de unit van Maarten; definitieve plaats met goed WiFi-bereik.
3. Optioneel, als Maarten wil: koppeling met Google Home via de Matterbridge op de RPi (zie de uitleg voor Maarten).

## 4. Praktisch

- Board **ESP32 Dev Module** (niet ESP32-WROOM-DA: die gebruikt GPIO 25 als antenneschakelaar). Geen extra bibliotheken.
- OTA: enkel `ESP32_PWM_Test.ino.bin` uit `build/esp32.esp32.esp32/` via `/update`. Een partitieschema zonder OTA werkt niet.
