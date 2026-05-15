#ifndef CANAL_H  // Guardia de inclusión
#define CANAL_H

#include "freertos/FreeRTOS.h"  // Incluir FreeRTOS
#include "freertos/semphr.h"  // Incluir semáforos

// LEDs del canal
#define LED_CANAL_INICIO   5    // Primer LED del canal
#define LED_CANAL_FIN      9   // Último LED del canal
#define LED_CANAL_TOTAL    5    // Cantidad de LEDs (6,7,8,9,10)

// LEDs reservados para interrupciones
#define LED_BARRERA_IZQ    4  // Barrera izquierda
#define LED_BARRERA_DER    10  // Barrera derecha

// LEDs de los océanos
#define LED_OCEANO_IZQ_INICIO   0  // Inicio océano izquierdo
#define LED_OCEANO_IZQ_FIN      3  // Fin océano izquierdo
#define LED_OCEANO_DER_INICIO   11  // Inicio océano derecho
#define LED_OCEANO_DER_FIN      14  // Fin océano derecho

// Estructura para posición del canal con mutex
typedef struct {
    SemaphoreHandle_t mutex;  // Mutex para la posición
} PosicionCanal;

// Estructura del canal
typedef struct {
    PosicionCanal posiciones[LED_CANAL_TOTAL];  // Array de posiciones
} Canal;

extern Canal canal;  // Declaración externa del canal

// Inicializar todas las posiciones como libres
void canal_init(void);

// Convertir LED real (6..10) → índice (0..4)
int canal_pos_a_index(int led_real);

// Ocupar/liberar una posición por su LED real
void canal_ocupar_posicion(int indice);
void canal_liberar_posicion(int indice);

#endif // CANAL_H
