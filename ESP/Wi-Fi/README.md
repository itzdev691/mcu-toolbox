# 📡 ESP32 WiFi Scripts

A collection of WiFi-related scripts for ESP32 microcontrollers. These examples demonstrate how ESP32 devices can connect to wireless networks, discover nearby access points, and communicate with other devices over WiFi. The C++ examples are dual-framework and can be built with **both ESP-IDF and the Arduino IDE.**

The goal of this folder is to provide simple, practical examples that can be used as building blocks for larger networking and IoT projects. Each script focuses on a specific WiFi feature so developers can quickly understand the concept and adapt the code for their own applications.

### ✨ Features

* Connect an ESP32 to a WiFi network
* Scan and list nearby WiFi networks
* Basic network communication examples
* AP+STA repeater starter with NAT/forwarding (ESP-IDF build)
* Lightweight scripts designed for experimentation and learning
* ESP-NOW Protocol

### 🧠 Purpose

These scripts are intended to act as reference implementations within this MCU repository. They help demonstrate how wireless networking can be implemented on ESP32 devices and provide a starting point for projects involving IoT, device communication, or embedded networking.

### 🛠 Requirements

* ESP32 development board
* Arduino IDE or ESP-IDF with ESP32 board support installed
* Basic familiarity with Arduino sketches

### 📝 Notes

* For `C++/wifi_repeater.cpp` in ESP-IDF, enable `CONFIG_LWIP_IP_FORWARD` and
  `CONFIG_LWIP_IPV4_NAPT` in menuconfig to activate NAT/forwarding.

---

Part of the **MCU-Toolbox**, which provides examples
