/**
 * Mapa de LEDs:
 *   0  - 3  : Océano Izquierdo (cola de espera)
 *   4        : Barrera física izquierda
 *   5        : RESERVADO para interrupción (aguja izquierda)
 *   6  - 10  : Canal (5 LEDs donde cruzan los barcos)
 *   11       : RESERVADO para interrupción (aguja derecha)
 *   12 - 15  : Océano Derecho (cola de espera)
 */

#ifndef CANAL_H
#define CANAL_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// ── LEDs del canal ──
#define LED_CANAL_INICIO   5    // Primer LED del canal
#define LED_CANAL_FIN      9   // Último LED del canal
#define LED_CANAL_TOTAL    5    // Cantidad de LEDs (6,7,8,9,10)

// ── LEDs reservados para interrupciones (no tocar) ──
#define LED_BARRERA_IZQ    4
#define LED_BARRERA_DER    10

// ── LEDs de los océanos ──
#define LED_OCEANO_IZQ_INICIO   0
#define LED_OCEANO_IZQ_FIN      3
#define LED_OCEANO_DER_INICIO   11
#define LED_OCEANO_DER_FIN      14

// Un mutex por LED del canal: taken = ocupado, given = libre
typedef struct {
    SemaphoreHandle_t mutex;
} PosicionCanal;

typedef struct {
    PosicionCanal posiciones[LED_CANAL_TOTAL];
} Canal;

extern Canal canal;

// Inicializar todas las posiciones como libres
void canal_init(void);

// Convertir LED real (6..10) → índice (0..4)
int canal_pos_a_index(int led_real);

// Ocupar/liberar una posición por su LED real
void canal_ocupar_posicion(int indice);
void canal_liberar_posicion(int indice);

#endif // CANAL_H
