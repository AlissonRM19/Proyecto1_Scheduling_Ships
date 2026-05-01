#include <Adafruit_NeoPixel.h>
#include "led_wrapper.h" // Incluimos el .h que acabamos de crear
#include "config.h"

// Usamos NUM_LEDS definido en tu config.h
Adafruit_NeoPixel tira(64, PIN_LED, NEO_GRB + NEO_KHZ800); 

extern "C" void led_init() {
    tira.begin();
    tira.clear();
    tira.show();
}

extern "C" void led_set_pixel(int index, int r, int g, int b) {
    if (index >= 0 && index < 64) {
        tira.setPixelColor(index, tira.Color(r, g, b));
    }
}

extern "C" void led_show() {
    tira.show();
}

extern "C" void led_clear() {
    tira.clear();
    tira.show();
}
