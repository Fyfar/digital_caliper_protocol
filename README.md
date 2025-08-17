# Caliper to OLED Display

This is a fun little project for anyone who wants to mess around with digital calipers and microcontrollers. I built this using an ESP32 board I had lying around to read data from a cheap digital caliper and display the measurements on an OLED screen.

## How It Works

The project uses a simple 2-wire protocol to communicate with the caliper. It reads a 24-bit packet, parses the data, and then sends the parsed value to an I2C OLED display.
* **Caliper Interface:** The caliper uses a simple CLK (clock) and DATA line. The code reads data on the rising edge of the clock signal.
* **Data Parsing:** The 24-bit packet contains information about the measurement, including the value, sign, and units (inches/mm). The firmware decodes this information.
* **Display:** The final measurement is shown on a 128x64 pixel OLED display.

## Hardware

* **ESP32 Development Board:** I used this because I had one available, but you could likely adapt the code for an Arduino or any other microcontroller with a bit of tinkering.
* **Digital Caliper:** A standard, inexpensive digital caliper. You'll need to open it up to access the data pins.
* **128x32 OLED Display (I2C):** A common small display for microcontroller projects.
* **Wires and Breadboard:** For connecting everything.

## Getting Started

1. **Clone this repository:** `git clone https://github.com/your-username/your-repo-name.git`
2. **Open the project in Arduino IDE.**
3. **Install the necessary libraries:**
   * `GyverOLED library`
4. **Connect your hardware:**
   * **Caliper DATA pin** to a digital pin on the ESP32 (e.g., GPIO 27).
   * **Caliper CLK pin** to a digital pin on the ESP32 (e.g., GPIO 26).
   * **OLED VCC** to 3.3V on the ESP32.
   * **OLED GND** to GND on the ESP32.
   * **OLED SDA** to the SDA pin on the ESP32 (usually GPIO 21).
   * **OLED SCL** to the SCL pin on the ESP32 (usually GPIO 22).
5. **Upload the code to your ESP32 board.**

## Future Plans

This is a work in progress! I plan to add:
  * A description of the 3D-printed enclosure.
  * A full list of components with links.
  * A more detailed explanation of the data protocol.
Photos and schematics.

---

Feel free to open issues or contribute if you have ideas on how to improve this project!
