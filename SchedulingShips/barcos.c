#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "barcos.h"
#include "canal.h"
#include "config.h"
#include "scheduler.h"

int contadorBarcos = 1;

extern int                barcos_en_canal_count;
extern int                sentido_actual;
extern Barco*             barcos_activos[MAX_BARCOS_CANAL];
extern SemaphoreHandle_t  sem_espacio_canal;
extern SemaphoreHandle_t  doneSemaphore;

const char* nombreTipo(int tipo) {
    switch (tipo) {
        case 1:  return "Normal";
        case 2:  return "Pesquero";
        case 3:  return "Patrulla";
        default: return "Desconocido";
    }
}

const char* nombreLado(int lado) {
    return (lado == 0) ? "Izquierda" : "Derecha";
}

// ===== TAREA DEL BARCO =====
void barcoTask(void *pvParameters) {
    Barco *barco      = (Barco *) pvParameters;
    int slot_asignado = -1;

    xSemaphoreTake(barco->sem, portMAX_DELAY);

    for (int i = 0; i < MAX_BARCOS_CANAL; i++) {
        if (barcos_activos[i] == NULL) {
            barcos_activos[i] = barco;
            slot_asignado     = i;
            break;
        }
    }

    printf("[Barco %d] %s desde %s entra al canal\n",
           barco->id, nombreTipo(barco->tipo), nombreLado(barco->lado));

    while (barco->tiempoRestante > 0) {

        // RR/STRN: esperar re-despacho tras cada slice
        if (barco->tiempoRestante < barco->tiempoCruce) {
            xSemaphoreTake(barco->sem, portMAX_DELAY);
        }

        int slice = barco->tiempoRestante;
        if (SCHEDULER_ACTUAL == SCHEDULER_RR && slice > RR_QUANTUM) {
            slice = RR_QUANTUM;
        }

        // Dirección: LEDs reales 6..10
        int led_inicio, led_fin, paso;
        if (barco->lado == 0) {        // Izquierda → Derecha
            led_inicio = LED_CANAL_INICIO;  // 6
            led_fin    = LED_CANAL_FIN;     // 10
            paso       = 1;
        } else {                        // Derecha → Izquierda
            led_inicio = LED_CANAL_FIN;     // 10
            led_fin    = LED_CANAL_INICIO;  // 6
            paso       = -1;
        }

        // Tiempo por LED según velocidad propia del barco
        int delay_por_led = (barco->tiempoCruce * 1000) / LED_CANAL_TOTAL;

        // LEDs a recorrer en este slice (para RR/STRN parciales)
        int leds_en_slice = (LED_CANAL_TOTAL * slice) / barco->tiempoCruce;
        if (leds_en_slice < 1) leds_en_slice = 1;
        if (leds_en_slice > LED_CANAL_TOTAL) leds_en_slice = LED_CANAL_TOTAL;

        // LED de inicio de este slice:
        // - Primer slice: empieza en led_inicio (6 o 10)
        // - Slices siguientes RR/STRN: continúa desde donde quedó
        int led_actual = (barco->tiempoRestante == barco->tiempoCruce)
                         ? led_inicio
                         : barco->posicion;

        // Ocupar primer LED antes de entrar al canal
        canal_ocupar_posicion(canal_pos_a_index(led_actual));
        barco->posicion = led_actual;
        printf("GUI:POS,%d,%d\n", barco->id, barco->posicion);

        for (int s = 0; s < leds_en_slice; s++) {

            vTaskDelay(pdMS_TO_TICKS(delay_por_led));

            int led_siguiente = led_actual + paso;
            bool hay_siguiente = (paso == 1)
                                  ? (led_siguiente <= led_fin)
                                  : (led_siguiente >= led_fin);

            if (hay_siguiente) {
                // Ocupar siguiente ANTES de liberar actual
                // Si hay un barco más lento adelante, bloquea aquí hasta que avance
                canal_ocupar_posicion(canal_pos_a_index(led_siguiente));
                canal_liberar_posicion(canal_pos_a_index(led_actual));

                led_actual      = led_siguiente;
                barco->posicion = led_actual;  // LED real para hardware.c
                printf("GUI:POS,%d,%d\n", barco->id, barco->posicion);
            } else {
                // Último LED: liberar y salir
                canal_liberar_posicion(canal_pos_a_index(led_actual));
            }
        }

        barco->tiempoRestante -= slice;
        xSemaphoreGive(doneSemaphore);
    }

    printf("GUI:DONE,%d\n", barco->id);
    printf("[Barco %d] Cruce completado\n", barco->id);

    if (slot_asignado != -1) {
        barcos_activos[slot_asignado] = NULL;
    }

    barcos_en_canal_count--;
    xSemaphoreGive(sem_espacio_canal);

    vSemaphoreDelete(barco->sem);
    free(barco);
    vTaskDelete(NULL);
}

// ===== CREAR BARCO =====
void crearBarco(int tipo, int lado) {
    Barco *nuevoBarco = (Barco*) malloc(sizeof(Barco));
    if (nuevoBarco == NULL) {
        printf("[ERROR] Sin memoria para barco\n");
        return;
    }

    // Posición inicial: LED 4 (barrera izq) o LED 12 (inicio océano der)
    // Solo referencial, hardware.c no pinta esta posición en el canal
    nuevoBarco->posicion = (lado == 0) ? 4 : 12;
    nuevoBarco->id       = contadorBarcos++;
    nuevoBarco->tipo     = tipo;
    nuevoBarco->lado     = lado;

    nuevoBarco->sem = xSemaphoreCreateBinary();
    if (nuevoBarco->sem == NULL) {
        printf("[ERROR] Sin semáforo para barco\n");
        free(nuevoBarco);
        return;
    }

    switch (tipo) {
        case 1: nuevoBarco->tiempoCruce = 5; nuevoBarco->prioridad = 1; break;
        case 2: nuevoBarco->tiempoCruce = 4; nuevoBarco->prioridad = 2; break;
        case 3: nuevoBarco->tiempoCruce = 2; nuevoBarco->prioridad = 3; break;
        default: nuevoBarco->tiempoCruce = 5; nuevoBarco->prioridad = 1; break;
    }

    nuevoBarco->tiempoRestante = nuevoBarco->tiempoCruce;
    nuevoBarco->deadline       = (int)(pdTICKS_TO_MS(xTaskGetTickCount()) / 1000)
                                 + (rand() % 20 + 10);

    if (!enqueueBarco(nuevoBarco)) {
        printf("[WARN] Cola llena, barco %d descartado\n", nuevoBarco->id);
        vSemaphoreDelete(nuevoBarco->sem);
        free(nuevoBarco);
        return;
    }

    printf("Barco %d agregado | tipo: %s | lado: %s\n",
           nuevoBarco->id, nombreTipo(tipo), nombreLado(lado));
    printf("GUI:ADD,%d,%d,%d,%d,%d,%d\n",
           nuevoBarco->id, nuevoBarco->tipo, nuevoBarco->lado,
           nuevoBarco->prioridad, nuevoBarco->tiempoRestante, nuevoBarco->deadline);

    UBaseType_t taskPriority = (tipo == 3) ? 5 : (tipo == 2) ? 2 : 1;
    xTaskCreate(barcoTask, "Barco", 4096, nuevoBarco, taskPriority, NULL);
}
