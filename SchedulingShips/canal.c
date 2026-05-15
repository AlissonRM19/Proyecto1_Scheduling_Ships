#include "canal.h"  // Incluir cabecera del canal
#include <stdio.h>  // Librería estándar para entrada/salida

Canal canal;  // Instancia global del canal

void canal_init(void) {  // Función para inicializar el canal
    for (int i = 0; i < LED_CANAL_TOTAL; i++) {  // Para cada posición del canal
        canal.posiciones[i].mutex = xSemaphoreCreateBinary();  // Crear semáforo binario
        xSemaphoreGive(canal.posiciones[i].mutex);  // Dar el semáforo, posición libre
    }
    printf("[Canal] %d posiciones inicializadas (LEDs %d-%d)\n",
           LED_CANAL_TOTAL, LED_CANAL_INICIO, LED_CANAL_FIN);  // Imprimir mensaje de inicialización
}

// Función para convertir LED real a índice
int canal_pos_a_index(int led_real) {
    return led_real - LED_CANAL_INICIO;  // Restar el inicio para obtener índice
}

void canal_ocupar_posicion(int indice) {  // Función para ocupar una posición
    if (indice < 0 || indice >= LED_CANAL_TOTAL) return;  // Verificar límites
    xSemaphoreTake(canal.posiciones[indice].mutex, portMAX_DELAY);  // Tomar el mutex
}

void canal_liberar_posicion(int indice) {  // Función para liberar una posición
    if (indice < 0 || indice >= LED_CANAL_TOTAL) return;  // Verificar límites
    xSemaphoreGive(canal.posiciones[indice].mutex);  // Dar el mutex
}
