#include <Arduino.h>
#include <FastLED.h>

constexpr uint16_t NUM_LEDS = 90;
constexpr uint8_t DATA_PIN = 2;
constexpr uint8_t BRIGHTNESS = 160;

CRGB leds[NUM_LEDS];

enum State { WAIT_A, WAIT_D, WAIT_A2, COUNT_HIGH, COUNT_LOW, CHECKSUM, PIXELS };
State state = WAIT_A;
uint8_t countHigh, countLow, expectedChecksum;
uint16_t expectedBytes = 0, received = 0;
uint8_t frame[NUM_LEDS * 3];

void setup() {
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.clear(true);
    Serial.begin(921600);
}

void acceptByte(uint8_t b) {
    switch (state) {
        case WAIT_A: state = (b == 'A') ? WAIT_D : WAIT_A; break;
        case WAIT_D: state = (b == 'd') ? WAIT_A2 : WAIT_A; break;
        case WAIT_A2: state = (b == 'a') ? COUNT_HIGH : WAIT_A; break;
        case COUNT_HIGH: countHigh = b; state = COUNT_LOW; break;
        case COUNT_LOW: countLow = b; expectedChecksum = countHigh ^ countLow ^ 0x55; state = CHECKSUM; break;
        case CHECKSUM:
            if (b == expectedChecksum && (((uint16_t)countHigh << 8 | countLow) + 1) == NUM_LEDS) { expectedBytes = NUM_LEDS * 3; received = 0; state = PIXELS; }
            
            else state = WAIT_A;
            break;
        case PIXELS:
        frame[received++] = b;
        if (received == expectedBytes) {
            for (uint16_t i = 0; i < NUM_LEDS; ++i) leds[i].setRGB(frame[3*i], frame[3*i + 1], frame[3*i + 2]);
            FastLED.show();
            state = WAIT_A;
        }

        break;
    }
}

void loop() {
    while (Serial.available()) acceptByte((uint8_t)Serial.read());
}