#include <Arduino.h>
#include <FastLED.h>

constexpr uint16_t NUM_LEDS = 90;
constexpr uint8_t DATA_PIN = 2;
constexpr uint8_t BRIGHTNESS = 160;

CRGB leds[NUM_LEDS];

enum State { WAIT_A, WAIT_D, WAIT_A2, PIXELS };
State state = WAIT_A;
uint16_t receivedBytes = 0;

void setup() {
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.clear(true);
    Serial.begin(921600);
}

void acceptByte(uint8_t b) {
    switch (state) {
        case WAIT_A2: state = (b == 'A') ? WAIT_D : WAIT_A; break;
        case WAIT_D: state = (b == 'd') ? WAIT_A2 : WAIT_A; break;
        case WAIT_A:
            if (b == 'a') {
                receivedBytes = 0;
                state = PIXELS;
            }

            else {
                state = WAIT_A;
            }
            break;

        case PIXELS:
        ((uint8_t*)leds)[receivedBytes++] = b;

        if (receivedBytes == NUM_LEDS * 3) {
            FastLED.show();
            state = WAIT_A;
        }
        break;
    }
}

void loop() {
}