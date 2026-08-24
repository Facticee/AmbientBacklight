#include <Arduino.h>
#include <FastLED.h>

constexpr uint16_t NUM_LEDS = 90;
constexpr uint8_t DATA_PIN = 2;
constexpr uint8_t BRIGHTNESS = 160;

CRGB leds[NUM_LEDS];

void setup() {
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.clear(true);
    Serial.begin(921600);
}

void loop() {
    if (Serial.available() >= 3) {
        if (Serial.read() == 'A' && Serial.read() == 'd' && Serial.read() == 'a') {
            size_t loaded = Serial.readBytes((char*)leds, NUM_LEDS * 3);

            if (loaded == NUM_LEDS * 3) {
                FastLED.show();
            }
        }
    }
}