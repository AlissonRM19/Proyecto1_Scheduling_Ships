#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h" // Driver nativo para el ESP32-C6

#include "hardware.h"
#include "barcos.h"
#include "config.h"

// Función auxiliar para configurar los pines de entrada
static void configurar_boton(int pin) {
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
}

void iniciarHardware() {
    printf("[Hardware] Inicializando GPIOs y LEDs...\n");

    // Configurar todos los botones como entrada con Pull-up
    configurar_boton(BOTON_NORMAL_IZQ);
    configurar_boton(BOTON_PESQUERO_IZQ);
    configurar_boton(BOTON_PATRULLA_IZQ);
    configurar_boton(BOTON_NORMAL_DER);
    configurar_boton(BOTON_PESQUERO_DER);
    configurar_boton(BOTON_PATRULLA_DER);

    // Inicializar el wrapper de los LEDs (definido en led_wrapper.cpp)
    led_init(); 
}

void mostrarTipoBarco(int tipo) {
    led_clear();
    // Colores: (R, G, B)
    if (tipo == 1) led_set_color(0, 0, 255);      // Normal -> Azul
    else if (tipo == 2) led_set_color(0, 255, 0); // Pesquero -> Verde
    else if (tipo == 3) led_set_color(255, 0, 0); // Patrulla -> Rojo
}

void leerBotones() {
    // Definimos los botones en una estructura para iterar fácilmente
    static const struct {
        int pin;
        int tipo;
        int lado;
        const char* nombre;
    } botones[] = {
        {BOTON_NORMAL_IZQ,   1, 0, "Normal Izquierda"},
        {BOTON_PESQUERO_IZQ, 2, 0, "Pesquero Izquierda"},
        {BOTON_PATRULLA_IZQ, 3, 0, "Patrulla Izquierda"},
        {BOTON_NORMAL_DER,   1, 1, "Normal Derecha"},
        {BOTON_PESQUERO_DER, 2, 1, "Pesquero Derecha"},
        {BOTON_PATRULLA_DER, 3, 1, "Patrulla Derecha"}
    };

    for (int i = 0; i < 6; i++) {
        // En el driver de ESP32, 0 significa presionado (debido al Pull-up)
        if (gpio_get_level(botones[i].pin) == 0) {
            printf("[Hardware] Pulsado: %s\n", botones[i].nombre);
            
            mostrarTipoBarco(botones[i].tipo);
            crearBarco(botones[i].tipo, botones[i].lado);
            
            // Un pequeño delay para evitar rebotes (Debounce)
            // pdMS_TO_TICKS convierte milisegundos a ticks de FreeRTOS
            vTaskDelay(pdMS_TO_TICKS(500)); 
        }
    }
}
