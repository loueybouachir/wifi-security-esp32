# WiFi Security Analyzer & Attack Simulator (ESP32)

This open-source project was developed as part of a Final Year Project (FYP) in Computer Science.  
It includes two embedded systems modules designed to evaluate the security of wireless networks and demonstrate common attack scenarios in controlled, educational settings using ESP32 boards.

---
## ⓘ For Full Project Documentation [clic here](docs/index.md)

## 📦 Project Overview

### 1. 📡 WiFi Analyzer (`wifi-analyzer/`)
- Scans nearby WiFi networks using the ESP32.
- Detects:
  - Open networks
  - Weak security protocols (WEP, WPA)
  - Hidden SSIDs
  - Unauthorized channels (above 11)
  - Evil Twin access points
  - Deauthentication attcks 
  - Network changes (new, disappeared, security changes)
- Stores results in SPIFFS
- Provides output via CLI or embedded web server 
- Sends WhatsApp Notifications
- Provides Audio-visual alerts (via LEDs and Buzzer)
- Provides real-time display (via LCD display)

### 2. 💣 Attack Simulator (`attack-simulator/`)
- Simulates classic WiFi-based attack scenarios (for **educational use only**):
  - Evil Twin simulation
  - Deauthentication simulation 
  - Open network simulation 
  - Unauthorized channel simulation 
  - Hidden network simulation  	
- Uses a second ESP32 board with an interactive menu to trigger simulations

---

## 🧰 Requirements

- Windows 10
- 2x ESP32 development board (e.g., ESP-WROOM-32)
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
- Python 3.x
- Git for Windows
- LCD Display (I2C, e.g., LCM1602)
- LEDs (two different colours)
- Active Buzzer

---

## 🚀 Getting Started

### Clone the Repository

```bash
git clone https://github.com/loueybouachir/wifi-security-esp32.git
cd wifi-security-esp32
`````

### Build the WiFi Analyzer

```bash
cd wifi-analyzer
idf.py set-target esp32
idf.py menuconfig     # Optional: configure WiFi channel or scan interval
idf.py build
idf.py -p COMx flash
`````

### Build the Attack Simulator

```bash
cd attack-simulator
idf.py set-target esp32
idf.py build
idf.py -p COMx flash
`````

## 🗂️ Project Structure
```bash
wifi-security-esp32/
├── wifi-analyzer/          # Source code for WiFi scanning & analysis 
├── attack-simulator/       # Code for educational attack simulation 
├── docs/                   # Technical documentation and diagrams 
├── LICENSE                 # License file (MIT) 
├── README.md               # This file 
└── .gitignore              # Git exclusions
`````

## 📂 Module Documentation
For detailed setup, features, and usage instructions of each component, please refer to the dedicated README files located in their respective directories:

- wifi-analyzer/README.md: Full documentation for the WiFi scanning and analysis module.

- attack-simulator/README.md: Details of the attack simulation scenarios and usage instructions.

- Each README includes hardware requirements, build steps, configuration tips, and usage disclaimers.

⚠️ This software is intended strictly for educational and academic research purposes.
All attack simulation functionalities must only be used in isolated test environments such as university labs or virtualized WiFi sandboxes.
Any use of this code in unauthorized or real-world networks is strictly prohibited and may violate local or international laws.
The authors and contributors are not liable for any misuse of the project.

## 🧑‍💻 Authors & Contributors

- Name: Louey Bouachir
- Email: louey.bouachir@gmail.com
- University: Sciences Faculty of Bizerte, Tunisia
- Supervisor : Dr. Litayem Nabil

## 📜 License
This project is licensed under the MIT License.
You are free to use, modify, and share it, provided that proper credit is given and the license file is included.

See the LICENSE file for full terms.
