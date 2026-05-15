#include <Adafruit_NeoPixel.h>  // Incluir Adafruit NeoPixel
#include "led_wrapper.h" // Incluimos el .h que acabamos de crear  // Incluir header
#include "config.h"  // Incluir configuración

// Usamos NUM_LEDS definido en tu config.h
Adafruit_NeoPixel tira(64, PIN_LED, NEO_GRB + NEO_KHZ800);  // Instancia de tira LED

extern "C" void led_init() {  // Inicializar LEDs
    tira.begin();  // Comenzar tira
    tira.clear();  // Limpiar
    tira.show();  // Mostrar
}

extern "C" void led_set_pixel(int index, int r, int g, int b) {  // Establecer pixel
    if (index >= 0 && index < 64) {  // Verificar índice
        tira.setPixelColor(index, tira.Color(r, g, b));  // Establecer color
    }
}

extern "C" void led_show() {  // Mostrar LEDs
    tira.show();  // Mostrar tira
}

extern "C" void led_clear() {  // Limpiar LEDs
    tira.clear();  // Limpiar tira
    tira.show();  // Mostrar
}
