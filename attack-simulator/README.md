# 💣 Attack Simulator (ESP32)

This module is part of the **WiFi Security Analyzer & Attack Simulator** project.  
It simulates real-world WiFi attack scenarios in **safe, isolated environments** for educational and research purposes. This tool is intended to help students and researchers better understand wireless vulnerabilities and security risks.

---

## 🎯 Features

- Interactive attack simulation using ESP32
- Triggered via physical menu selection
- Simulates:
  - 📡 **Evil Twin Access Points** (duplicate SSID with fake BSSID)
  - 🔄 **Deauthentication Attacks** (disconnects clients)
  - 🔓 **Open Network Simulation** (unencrypted SSID)
  - 📶 **Unauthorized Channel Use** (e.g., channels >11)
  - 🙈 **Hidden Network Simulation** (SSID broadcast disabled)
- Supports dynamic attack switching in runtime
- Ideal for university labs and training workshops

---

## 🧰 Hardware Requirements

- 1x ESP32 development board  
- Access to a controlled WiFi environment  
- Micro-USB cable for flashing  

---

## 🛠️ Software Requirements

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
- Python 3.x
- Git for Windows

---

## 🚀 How to Build and Flash

```bash
# Navigate to this subfolder
cd attack-simulator

# Set the ESP32 target
idf.py set-target esp32

# Build the firmware
idf.py build

# Flash the firmware to the ESP32 (replace COMx with your actual port)
idf.py -p COMx flash
````` 
## 🧑‍🏫 How It Works
Once flashed, the ESP32 will start in idle mode. You can select a type of attack via menu system consol.
Each simulation mimics specific behaviors observed in real-world WiFi attacks without harming real networks when used in isolated environments.

## 📌 Notes
● ESP-IDF does not allow the alteration of management frames. To bypass this, a sanity check must be redefined. See line 43 in main.c. One must compile the program with build flags -Wl or -zmuldefs. This is necessary for the redefinition of this sanity check.

● Simulated SSIDs and MAC addresses can be configured in the source code.

● All attack behaviors are non-destructive and are meant for controlled lab conditions.

● Ensure your test WiFi environment is isolated from production or public networks.

● No data is collected or transmitted to external servers.

## ⚠️ Educational Use Disclaimer
This module is intended strictly for educational and academic research purposes.
It must only be used in controlled test environments, such as university labs or WiFi sandboxes.
Using this tool in unauthorized networks may violate local or international laws.
The authors and contributors assume no responsibility for misuse.

## 📜 License
Licensed under the MIT License.
You are free to use, modify, and distribute this software under the terms of the license.