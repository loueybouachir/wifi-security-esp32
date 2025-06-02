# 📡 WiFi Analyzer (ESP32)

This module is part of the **WiFi Security Analyzer & Attack Simulator** project.  
It is responsible for scanning nearby WiFi networks, analyzing their security configurations, and detecting vulnerabilities in real time using the ESP32 platform.

---
## ⓘ For Full Project Documentation [clic here](../docs/index.me)

## 🎯 Features

- Scans and lists nearby WiFi access points
- Detects:
  - Open networks (no encryption)
  - Weak security protocols (WEP, WPA)
  - Hidden SSIDs (broadcast disabled)
  - Use of unauthorized channels (>11)
  - Evil Twin access points (duplicate SSID, different BSSID)
  - Deauthentication attacks
  - Appearance and disappearance of networks
  - Changes in security or RSSI
- Stores results in SPIFFS
- CLI-based display or HTML rendering via embedded web server
- Sends real-time **WhatsApp notifications**
- LCD display for visual feedback (via I2C module)
- LED (Green/Red) and Buzzer alerts for risky networks

---

## 🧰 Hardware Requirements

- 1x ESP32 development board  
- LCD display (I2C, e.g., LCM1602)  
- 2x LEDs (green + red)  
- 1x Active Buzzer  
- WiFi access in test environment  
- Micro-USB cable for flashing  

---

## 🛠️ Software Requirements

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
- Python 3.x
- Git for Windows
- WhatsApp API key (for notification module)

---

## 🚀 How to Build and Flash

```bash
# Navigate to this subfolder
cd wifi-analyzer

# Set the ESP32 target
idf.py set-target esp32

# Optional: Configure scan interval or thresholds
idf.py menuconfig

# Build the firmware
idf.py build

# Flash the firmware to the ESP32 (replace COMx with your actual port)
idf.py -p COMx flash
`````
## 📌 Notes
● Final results are stored in /spiffs/comparative_report.csv and can be downloaded via the embedded web interface.

● Network comparison (to detect changes) is done every N cycles (configurable).

● Notifications are sent only when a critical threat is detected.

● Before compiling  set your local network SSID and PASSWORD in /main/app_status.c to ensure ESP32 WiFi connectivity.

● To enable WhatsApp alerts, set your PHONE_NUMBER, API_KEY, BOT_TOKEN, and CHAT_ID (CallMeBot API)

● If you want to switch to Telegram Bot visit /main/telegram_sender.c

● To visualize the final results in the web interface:
Open a browser connected to the same WiFi network as the ESP32 and visit:
http://<your-esp32-ip>/results

## ⚠️ Educational Use Disclaimer

This module is designed exclusively for educational and academic research purposes.
It must only be used in controlled environments such as university labs.
Unauthorized scanning or analysis of third-party networks may be illegal.
The authors and contributors assume no liability for misuse of this code.

## 📜 License

Licensed under the MIT License.
You are free to use, modify, and distribute this software under the terms of the license.
