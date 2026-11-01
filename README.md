# ESP32 Ambilight

A fast custom Ambilight setup for your PC monitor. It captures screen edge colors in real time and streams them over Serial directly to an ESP32 controlling a WS2812B LED strip.

## Features
- **Edge Capture:** Reads the average color in specific zones for each LED instead of only one pixel.
- **USB Serial:** Data transfer at 115200 Baud.

## CAD
I did use any CAD for this project because i simply dont need it, but if you want to design a case for it you can do that.

## Firmware & Software
- **ESP32:** Uses Arduino C++ with the `FastLED` library (or similar Adalight-compatible firmware) to map serial bytes to the LEDs.
- **PC:** Custom C++ application with Windows Desktop Duplication and a DirectX 11 Compute Shader (`shader.hlsl`) for low latenc screen capture and color processing and streaming frame data over Serial (`Win32 API`).

## BOM (Bill of Materials)
(More detailed list found in the BOM.csv file above - **Note that the components in the file are the things I boguht not including things I already had at home!**)
### Main components:

- 1x ESP32-S3-MINI-1 module
- 1x WS2812B LED strip (~90 LEDs)
- 1x 5V DC power supply, minimum 5A
- 1x USB-C data cable
- 1x Custom PCB made out of:


### PCB components:

- 1x CUI PJ-102AH DC barrel jack (5.5mm outer / 2.0mm center)
- 1x C&K OS102011MS2Q slide switch (Power on / off)
- 1x Littelfuse 0452005.MRL 5A fuse
- 1x TPS259480 eFuse
- 1x TPSM828224 3.3V buck converter
- 1x 74AHCT125 shifter
- 1x USBLC6-2SC6 USB ESD protection
- 1x Phoenix MKDS 1.5/3-5.08 LED output
- Resistors, capacitors
