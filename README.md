
# Boardoza LED Matrix Panel Interactive RGB Display Project

The **Boardoza LED Matrix Panel Project** is an ESP32-based programmable RGB display system featuring twelve interconnected 8 × 8 WS2812B LED matrices, forming a 32 × 24 pixel display with 768 individually addressable RGB LEDs. The system automatically cycles through five built-in animations — Smiley, Alien, Heart, Rainbow, and Scrolling Text — immediately after power-up, without requiring any external configuration or mobile application. Its custom LED mapping system ensures that animations, graphics, and text render correctly across all twelve chained panels.

The system supports customizable RGB colors, scrolling text, and a 32 × 24 pixel drawing canvas. Its Adafruit GFX-based coordinate mapping system handles panel orientation, mirroring, and LED chaining for accurate graphics rendering. The project is suitable for **STEM education, interactive exhibitions, retail displays, digital signage, and desktop decoration**.

<h2 align="center">LED Matrix Panel</h2>
<p align="center">
  <img src="./assets/Led Display.gif" alt="Boardoza LED Matrix Panel" width="400">
</p>

---

## Key Features

- **Automatic Animation Showcase:** Cycles through five built-in scenes — Smiley, Alien, Heart, Rainbow, and Scrolling Text — every 8 seconds after power-up, allowing standalone operation without an application.
- **Dynamic RGB Animation Engine:** Generates procedural animations featuring smooth movement, blinking effects, heartbeat-style brightness transitions, animated sprites, and a 256-step rainbow color wheel.
- **Custom LED Matrix Mapping:** Uses an Adafruit GFX-based coordinate mapping system to handle panel orientation, mirroring, and LED chaining, enabling seamless graphics across the complete 32 × 24 display.
- **Customizable Colors and Scrolling Text:** Provides independent RGB color control for seven graphical elements, five shared color-temperature presets, and continuously scrolling messages of up to 40 characters with Turkish character transliteration.
- **Interactive Pixel Drawing:** Features a 32 × 24 pixel drawing canvas with customizable RGB colors, batched pixel updates, and efficient canvas refreshing.

---

## System Architecture

The LED Matrix Panel system combines an ESP32-S3 controller, twelve interconnected 8 × 8 addressable RGB LED modules, and a USB Type-C converter to create a programmable and interactive RGB display.

| **Component** | **Model** |
| --- | --- |
| Microcontroller | [Boardoza ESP32-S3 Pulse](https://github.com/Boardoza/Boardoza_ESP32-S3_Pulse_Development_Board) |
| RGB LED Matrix | [Boardoza 8 × 8 ARGB Display Module](https://github.com/Boardoza/Boardoza_8x8_ARGB_Display_WS2812B_LED_Matrix_Breakout_Board) |
| Type-C Converter | [Boardoza USB Type-C](https://github.com/Boardoza/Boardoza_USB_Type_C_Breakout_Board) |
| Voltage Regulator | [Boardoza TPS62130A](https://github.com/Boardoza/Boardoza_TPS62130A_3V3_3A_Buck_Converter_Breakout_Board) |

---

## Board Pinout

The complete wiring diagram for the LED Matrix Panel project is provided below. It includes the connections between the ESP32-S3 controller, chained RGB LED matrices, and USB Type-C converter.

[Boardoza LED Display Pinout Diagram](./assets/LED%20Display%20Pinout%20Diagram.pdf)


---

## Step Files

[Boardoza LED Display.step](./assets/Led%20Display.step)

---

## Version History

- V1.0.0 - Initial Release

---

## Support

- If you have any questions or need support, please contact support@boardoza.com

---

## **License**

This repository contains both hardware and software components:

### **Hardware Design**

[![CC BY-SA 4.0][cc-by-sa-shield]][cc-by-sa]

All hardware design files are licensed under [Creative Commons Attribution-ShareAlike 4.0 International License][cc-by-sa].

[cc-by-sa]: http://creativecommons.org/licenses/by-sa/4.0/
[cc-by-sa-shield]: https://img.shields.io/badge/License-CC%20BY--SA%204.0-lightgrey.svg

### **Software/Firmware**

[![BSD-3-Clause][bsd-shield]][bsd]

All software and firmware are licensed under [BSD 3-Clause License][bsd].

[bsd]: https://opensource.org/licenses/BSD-3-Clause
[bsd-shield]: https://img.shields.io/badge/License-BSD%203--Clause-blue.svg
