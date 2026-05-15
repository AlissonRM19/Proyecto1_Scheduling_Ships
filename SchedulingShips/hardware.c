/**
 * hardware.c
 *
 * Correcciones respecto a la versión anterior:
 *   - Canal: LEDs 6-10 (no 5-10). El limpiado empieza en 6.
 *   - LEDs 5 y 11 RESERVADOS para interrupciones — nunca se tocan aquí.
 *   - Océano derecho: empieza en LED 12 (no 11).
 *   - barco->posicion ya contiene el LED real (6-10), sin conversión extra.
 *   - Validación de rango al pintar barcos en canal (6 <= pos <= 10).
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "led_wrapper.h"
#include "hardware.h"
#include "barcos.h"
#include "canal.h"
#include "config.h"
#include "scheduler.h"

#include "fcfs_scheduler.h"
#include "priority_scheduler.h"
#include "sjf_scheduler.h"
#include "rr_scheduler.h"
#include "strn_scheduler.h"
#include "edf_scheduler.h"

extern ConfigProyecto miConfig;
extern fcfs_scheduler_t     fcfs_Izq,  fcfs_Der;
extern priority_scheduler_t prio_Izq,  prio_Der;
extern sjf_scheduler_t      sjf_Izq,   sjf_Der;
extern rr_scheduler_t       rr_Izq,    rr_Der;
extern strn_scheduler_t     strn_Izq,  strn_Der;
extern edf_scheduler_t      edf_Izq,   edf_Der;
extern Barco*               barcos_activos[MAX_BARCOS_CANAL];

// ── Mapa de LEDs ──────────────────────────────────────────────
//  0- 3  Océano Izquierdo
//  4      Barrera física izquierda
//  5      RESERVADO — aguja izquierda (interrupciones)
//  6-10   Canal
//  11     RESERVADO — aguja derecha  (interrupciones)
//  12-15  Océano Derecho
// ─────────────────────────────────────────────────────────────

static void configurar_boton(int pin) {
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
}

// ===== INIT =====
void iniciarHardware() {
    printf("[Hardware] Inicializando GPIOs y LEDs...\n");

    configurar_boton(BOTON_NORMAL_IZQ);
    configurar_boton(BOTON_PESQUERO_IZQ);
    configurar_boton(BOTON_PATRULLA_IZQ);
    configurar_boton(BOTON_NORMAL_DER);
    configurar_boton(BOTON_PESQUERO_DER);
    configurar_boton(BOTON_PATRULLA_DER);

    led_init();
}

// ===== FEEDBACK VISUAL AL PRESIONAR BOTÓN =====
void mostrarTipoBarco(int tipo) {
    led_clear();
    if      (tipo == 1) led_set_pixel(0, 0,   0,   255); // Normal   → Azul
    else if (tipo == 2) led_set_pixel(0, 0,   255, 0);   // Pesquero → Verde
    else if (tipo == 3) led_set_pixel(0, 255, 0,   0);   // Patrulla → Rojo
    led_show();
}

// ===== HELPER: color por tipo de barco =====
static void color_por_tipo(int tipo, int *r, int *g, int *b) {
    *r = 0; *g = 0; *b = 0;
    if      (tipo == 1) *b = 255;   // Normal   → Azul
    else if (tipo == 2) *g = 255;   // Pesquero → Verde
    else if (tipo == 3) *r = 255;   // Patrulla → Rojo
}

// ===== ACTUALIZAR TIRA LED =====
void actualizar_tira_led() {
    led_clear(); // Limpiar TODO de una vez al inicio

    // ── Contar barcos en espera ──
    int esperandoIzq = 0, esperandoDer = 0;
    switch (SCHEDULER_ACTUAL) {
        case SCHEDULER_FCFS:     esperandoIzq = fcfs_Izq.count;  esperandoDer = fcfs_Der.count;  break;
        case SCHEDULER_PRIORITY: esperandoIzq = prio_Izq.count;  esperandoDer = prio_Der.count;  break;
        case SCHEDULER_SJF:      esperandoIzq = sjf_Izq.count;   esperandoDer = sjf_Der.count;   break;
        case SCHEDULER_RR:       esperandoIzq = rr_Izq.count;    esperandoDer = rr_Der.count;    break;
        case SCHEDULER_STRN:     esperandoIzq = strn_Izq.count;  esperandoDer = strn_Der.count;  break;
        case SCHEDULER_EDF:      esperandoIzq = edf_Izq.count;   esperandoDer = edf_Der.count;   break;
    }

    // ── SECCIÓN 1: Océano Izquierdo (LEDs 0-3) ──
    // Máximo 4 barcos visibles en la cola
    for (int i = 0; i < esperandoIzq && i < 4; i++) {
        Barco *b = NULL;

        switch (SCHEDULER_ACTUAL) {
            case SCHEDULER_FCFS: {
                int idx = (fcfs_Izq.head + i) % FCFS_SCHEDULER_MAX_ITEMS;
                b = (Barco*)fcfs_Izq.buffer[idx];
                break;
            }
            case SCHEDULER_RR: {
                int idx = (rr_Izq.head + i) % RR_SCHEDULER_MAX_ITEMS;
                b = (Barco*)rr_Izq.buffer[idx];
                break;
            }
            case SCHEDULER_PRIORITY: b = (Barco*)prio_Izq.buffer[i];  break;
            case SCHEDULER_SJF:      b = (Barco*)sjf_Izq.buffer[i];   break;
            case SCHEDULER_STRN:     b = (Barco*)strn_Izq.buffer[i];  break;
            case SCHEDULER_EDF:      b = (Barco*)edf_Izq.buffer[i];   break;
        }

        if (b != NULL) {
            int r, g, bcolor;
            color_por_tipo(b->tipo, &r, &g, &bcolor);
            led_set_pixel(i, r, g, bcolor); // LEDs 0,1,2,3
        }
    }

    // ── SECCIÓN 2: Canal (LEDs 6-10) ──
    // LED 5 y 11 NO se tocan — reservados para interrupciones

    // Limpiar solo los LEDs del canal
    for (int led = LED_CANAL_INICIO; led <= LED_CANAL_FIN; led++) {
        led_set_pixel(led, 0, 0, 0);
    }

    // Pintar barcos activos en el canal
    for (int i = 0; i < MAX_BARCOS_CANAL; i++) {
        Barco *b = barcos_activos[i];
        if (b == NULL) continue;

        // Validar que la posición esté dentro del canal (6-10)
        // Evita pintar en LEDs reservados si hay algún valor incorrecto
        if (b->posicion < LED_CANAL_INICIO || b->posicion > LED_CANAL_FIN) continue;

        int r, g, bcolor;
        color_por_tipo(b->tipo, &r, &g, &bcolor);
        led_set_pixel(b->posicion, r, g, bcolor);
    }

    // ── SECCIÓN 3: Océano Derecho (LEDs 12-15) ──
    // LED 11 NO se toca — reservado para interrupción
    for (int i = 0; i < esperandoDer && i < 4; i++) {
        Barco *b = NULL;

        switch (SCHEDULER_ACTUAL) {
            case SCHEDULER_FCFS: {
                int idx = (fcfs_Der.head + i) % FCFS_SCHEDULER_MAX_ITEMS;
                b = (Barco*)fcfs_Der.buffer[idx];
                break;
            }
            case SCHEDULER_RR: {
                int idx = (rr_Der.head + i) % RR_SCHEDULER_MAX_ITEMS;
                b = (Barco*)rr_Der.buffer[idx];
                break;
            }
            case SCHEDULER_PRIORITY: b = (Barco*)prio_Der.buffer[i];  break;
            case SCHEDULER_SJF:      b = (Barco*)sjf_Der.buffer[i];   break;
            case SCHEDULER_STRN:     b = (Barco*)strn_Der.buffer[i];  break;
            case SCHEDULER_EDF:      b = (Barco*)edf_Der.buffer[i];   break;
        }

        if (b != NULL) {
            int r, g, bcolor;
            color_por_tipo(b->tipo, &r, &g, &bcolor);
            led_set_pixel(LED_OCEANO_DER_INICIO + i, r, g, bcolor); // LEDs 12,13,14,15
        }
    }

    led_show(); // Enviar todo el frame de una vez
}

// ===== LEER BOTONES =====
void leerBotones() {
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
        if (gpio_get_level(botones[i].pin) == 0) { // 0 = presionado (pull-up)
            int r = 0, g = 0, b = 0;
            color_por_tipo(botones[i].tipo, &r, &g, &b);

            // Feedback visual en el LED del extremo correspondiente
            // Izquierda → LED 0, Derecha → LED 15
            int led_feedback = (botones[i].lado == 0)
                               ? LED_OCEANO_IZQ_INICIO
                               : LED_OCEANO_DER_FIN;
            led_set_pixel(led_feedback, r, g, b);
            led_show();

            crearBarco(botones[i].tipo, botones[i].lado);

            vTaskDelay(pdMS_TO_TICKS(300)); // Debounce

            led_clear();
            led_show();
        }
    }
}
