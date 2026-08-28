# ESP32 Ambilight

A fast custom Ambilight setup for your PC monitor. It captures screen edge colors in real time and streams them over Serial directly to an ESP32 controlling a WS2812B LED strip.

## Features
- **Edge Capture:** Reads the average color in specific zones for each LED instead of only one pixel.
- **USB Serial:** Data transfer at 115200 Baud.

## CAD
Since this project uses components, no custom 3D printed case or PCB was needed. Everything is mounted directly to the back of the monitor (or near it).
Here is the basic Circuit. I will change the breadboard with two WAGOs 221-413 (splicing connectors). If you want to see a closeup of the connections with the ESP32, look under CAD in main.
<img width="1270" height="817" alt="Screenshot 2026-08-28 013857" src="https://github.com/user-attachments/assets/fe401d4a-2a0f-4684-a0ed-863bae9ae42b" />

## Firmware & Software
- **ESP32:** Uses Arduino C++ with the `FastLED` library (or similar Adalight-compatible firmware) to map serial bytes to the LEDs.
- **PC:** Custom C++ application with Windows Desktop Duplication and a DirectX 11 Compute Shader (`shader.hlsl`) for low latenc screen capture and color processing and streaming frame data over Serial (`Win32 API`).

## BOM (Bill of Materials)
(More detailed list found in the BOM.csv file above - **Note that the components in the file are the things I boguht not including things I already had at home!**)
- 1x ESP32 Dev Board (NodeMCU / DevKit V1)
- 1x WS2812B LED Strip (Depending on the energy class, you might need a DC Power Supply with 6A rather than 5A)
- 1x DC 5V 5A Power Supply (5.5 x 2.5mm jack)
- 1x DC Barrel Jack Adapter (5.5 x 2.5mm)
- 2x WAGO 221 Lever Connectors (3-wire)
- 1x USB Data Cable (USB-C for ESP32)
- 1x Dupont Jumper Wires
- Double-sided foam tape & Electrical tape (optional)
