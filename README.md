# MicroPython-DB
MicroPython database that includes a wide variety of devices in it.
* Use MicroPython v.1.27.0!
 # How to install MicroPython?
## Method 1: Using the Thonny IDE (Recommended for Beginners) 
Thonny is a popular Python IDE that includes a built-in feature for flashing MicroPython firmware on many boards, including the Raspberry Pi Pico and various ESP32 and ESP8266 devices. 

* Install Thonny Download and install the latest version of the Thonny IDE.
Connect Your Board Connect your microcontroller to your computer via a USB cable. For Raspberry Pi Pico boards, you must press and hold the BOOTSEL button while plugging it in to enter mass storage mode.

Open the Installer
* In Thonny, go to Tools > Options > Interpreter tab.
Select the appropriate MicroPython interpreter for your device (e.g., "MicroPython (Raspberry Pi Pico)" or "MicroPython (ESP32)").
Click the hyperlink (usually in the bottom right of the options window) to "Install or update MicroPython firmware".
Flash the Firmware In the installer window, select the correct port, firmware version, and device variant, then click "Install". The tool handles downloading and flashing the .bin or .uf2 file.

## Method 2: Using Vendor-Specific GUI Tools
Some board manufacturers provide dedicated, easy-to-use GUI tools for flashing their specific boards. 
Arduino Boards For boards like the Arduino Nano ESP32, dedicated "Firmware Installer" applications are available from Arduino documentation that automate the process.
SparkFun Boards SparkFun offers a specific "SparkFun MicroPython Firmware Updater" tool for their compatible boards.
Drag-and-Drop (Raspberry Pi Pico) For the Raspberry Pi Pico, you can manually download the official MicroPython .uf2 file from the MicroPython downloads page and drag-and-drop it onto the RPI-RP2 volume that appears when connected in BOOTSEL mode
