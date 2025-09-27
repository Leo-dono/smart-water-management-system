# Smart Water Metering System (SWMS)

**One-line:** An Arduino-based smart water meter prototype that measures flow & totalizer, monitors water quality (pH, conductivity/TDS, turbidity, temperature), detects and locates pipeline leaks, performs pressure relief, and notifies users via GSM.

---

## Table of Contents
- Overview
- Key features
- Hardware (BOM)
- Software & libraries
- System modes & behaviour
- Wiring & calibration notes
- Build & upload instructions
- Repository layout
- Contributing & license

---

## Overview
SWMS is a prototype system for real-time water quality monitoring and leak localization. It integrates multiple sensors and solenoid valves controlled by an Arduino Mega (or Arduino Mega WiFi variant). The system can automatically close valves on leak or poor water quality and send SMS alerts to designated numbers. The design supports a hybrid power supply (adapter + battery/solar) and local display.

---

## Key features
- Continuous measurement: flow rate & totalizer, pH, turbidity, conductivity/TDS, temperature  
- Leakage detection + leak localization using pressure sensors and valve isolation  
- Pressure relief mode for overpressure events  
- GSM-based alerts and remote commands (SIM800L)  
- Local display (128×64 with I2C) and EEPROM persistence of totalizer  

---

## Hardware (Bill of Materials — essential)
- Arduino Mega (or Mega WiFi) — main controller
- Turbidity sensor
- Conductivity / TDS sensor
- pH sensor + probe + amplifier board
- Hall flow meter (pulse output)
- Pressure sensors ×3 (for pipeline sections)
- Solenoid valves (main + sectioning + pressure-relief)
- SIM800L GSM module with separate 2A 5V supply
- RTC module (for timestamps)
- 128×64 LCD with I2C converter
- Power: 12V supply + buck converters (one dedicated to GSM), optional solar & battery

---

## Software & required Arduino libraries
Install via Arduino Library Manager or PlatformIO:
- EEPROM (standard)
- U8g2 (display)
- TimeLib
- OneWire
- DallasTemperature (DS18B20)
- GravityTDS
- SoftwareSerial
- Wire / SPI as required

---

## System modes
- **Normal mode:** All valves open, sensors sample, local + remote data available  
- **Leakage detection:** Closes valves, checks pressure, isolates leaking section, sends alert  
- **Poor water quality:** If thresholds exceeded, closes valve and notifies  
- **Pressure relief:** Opens relief valve on overpressure  

---

## Wiring & calibration notes
- Adjust OneWire bus pin in code
- Calibrate pH sensor with pH 7 buffer, set calibration constant in code
- GSM module requires dedicated regulator (2A bursts)

---

## Build & upload
1. Install Arduino IDE 
2. Install listed libraries  
3. Open `firmware/capstonefinalcode.ino`, set board to Arduino Mega, select COM port  
4. Compile & upload  
5. Test each module before full integration  

---

## Repo layout
```
swms/
├─ README.md
├─ LICENSE
├─ .gitignore
├─ firmware/
│  └─ capstonefinalcode.ino
├─ hardware/
│  ├─ BOM.csv
│  ├─ schematics/
│  └─ photos/
├─ docs/
│  └─ thesis_swms.pdf
├─ examples/
└─ CONTRIBUTING.md
```

---

## License
MIT License (recommended)  
