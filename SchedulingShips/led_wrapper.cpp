#include <Adafruit_NeoPixel.h>
#include "hardware.h"
#include "config.h"

Adafruit_NeoPixel tira(NUM_LEDS, PIN_LED, NEO_GRB + NEO_KHZ800);

extern "C" void led_init() {
    tira.begin();
    tira.clear();
    tira.show();
}

extern "C" void led_set_color(int r, int g, int b) {
    tira.setPixelColor(0, tira.Color(r, g, b));
    tira.show();
}

extern "C" void led_clear() {
    tira.clear();
    tira.show();
}
