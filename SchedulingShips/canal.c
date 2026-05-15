#include "canal.h"
#include <stdio.h>

Canal canal;

void canal_init(void) {
    for (int i = 0; i < LED_CANAL_TOTAL; i++) {
        canal.posiciones[i].mutex = xSemaphoreCreateBinary();
        xSemaphoreGive(canal.posiciones[i].mutex); // libre al inicio
    }
    printf("[Canal] %d posiciones inicializadas (LEDs %d-%d)\n",
           LED_CANAL_TOTAL, LED_CANAL_INICIO, LED_CANAL_FIN);
}

// LED real 6→índice 0, 7→1, ... 10→4
int canal_pos_a_index(int led_real) {
    return led_real - LED_CANAL_INICIO;
}

void canal_ocupar_posicion(int indice) {
    if (indice < 0 || indice >= LED_CANAL_TOTAL) return;
    xSemaphoreTake(canal.posiciones[indice].mutex, portMAX_DELAY);
}

void canal_liberar_posicion(int indice) {
    if (indice < 0 || indice >= LED_CANAL_TOTAL) return;
    xSemaphoreGive(canal.posiciones[indice].mutex);
}
