#include <stdio.h>      // Para printf
#include <stdlib.h>     // Para malloc y free
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "barcos.h"
#include "config.h"
#include "scheduler.h"

int contadorBarcos = 1;

#define MAX_BARCOS_CANAL 5

// Funciones auxiliares para imprimir tipo y lado de los barcos
const char* nombreTipo(int tipo) {
    if (tipo == 1) return "Normal";
    if (tipo == 2) return "Pesquero";
    if (tipo == 3) return "Patrulla";
    return "Desconocido";
}

// Función para convertir el lado a un nombre legible
const char* nombreLado(int lado) {
    if (lado == 0) return "Izquierda";
    return "Derecha";
}

// ===== TAREA DEL BARCO =====
void barcoTask(void *pvParameters) {
    Barco *barco = (Barco *) pvParameters;
    int slot_asignado = -1;

    // Referencias a variables definidas en otro archivo (Linker las buscará)
    extern int barcos_en_canal_count;
    extern int sentido_actual;
    extern Barco* barcos_activos[MAX_BARCOS_CANAL];
    extern SemaphoreHandle_t sem_espacio_canal;
    extern SemaphoreHandle_t doneSemaphore;

    xSemaphoreTake(barco->sem, portMAX_DELAY); 

    // Registrar en el canal
    for (int i = 0; i < MAX_BARCOS_CANAL; i++) {
        if (barcos_activos[i] == NULL) {
            barcos_activos[i] = barco;
            slot_asignado = i;
            break;
        }
    }

    barcos_en_canal_count++;
    sentido_actual = barco->lado;

    // --- LÓGICA DE CRUCE (Soporta RR y Pipeline) ---
    // --- LÓGICA DE CRUCE VISUAL ---
    while (barco->tiempoRestante > 0) {
    
        // RR espera permiso nuevamente
        if (barco->tiempoRestante < barco->tiempoCruce) {
            xSemaphoreTake(barco->sem, portMAX_DELAY);
        }
    
        int slice = barco->tiempoRestante;
    
        if (SCHEDULER_ACTUAL == SCHEDULER_RR && slice > RR_QUANTUM) {
            slice = RR_QUANTUM;
        }
    
        // LEDs reales del canal
        int inicio;
        int fin;
        int paso;
    
        if (barco->lado == 0) {
            // IZQ -> DER
            inicio = 5;
            fin = 10;
            paso = 1;
        } else {
            // DER -> IZQ
            inicio = 10;
            fin = 5;
            paso = -1;
        }
    
        // Cantidad total de LEDs del canal
        int total_leds = 5;
    
        // Tiempo por LED
        int delay_por_led = (slice * 1000) / total_leds;
    
        // Movimiento visual
        for (int p = inicio;
             (paso == 1) ? (p <= fin) : (p >= fin);
             p += paso) {
    
            barco->posicion = p;
    
            vTaskDelay(pdMS_TO_TICKS(delay_por_led));
        }
    
        barco->tiempoRestante -= slice;
    
        // Avisar scheduler
        xSemaphoreGive(doneSemaphore);
    }

    printf("GUI:DONE,%d\n", barco->id);
    
    if (slot_asignado != -1) {
        barcos_activos[slot_asignado] = NULL;
    }

    barcos_en_canal_count--; // <--- Aquí es donde fallaba el Linker

    if (sem_espacio_canal != NULL) {
        xSemaphoreGive(sem_espacio_canal);
    }

    vSemaphoreDelete(barco->sem);
    free(barco);
    vTaskDelete(NULL);
}



// ===== CREAR BARCO =====
// Esta función se llama desde la tarea generadora para crear un nuevo barco y agregarlo al scheduler
void crearBarco(int tipo, int lado) {
    Barco *nuevoBarco = (Barco*) malloc(sizeof(Barco));

    // Verificación de memoria
    if (nuevoBarco == NULL) {
        printf("Error reservando memoria\n");
        return;
    }
    nuevoBarco->posicion = (lado == 0) ? 4 : 11; // Espera en los extremos

    // Inicialización del barco
    // Asignamos un ID único al barco y configuramos sus atributos según el tipo
    nuevoBarco->id = contadorBarcos++;
    nuevoBarco->tipo = tipo;
    nuevoBarco->lado = lado;

    // Crear un semáforo binario para que el barco espere a ser programado
    nuevoBarco->sem = xSemaphoreCreateBinary();

    if (nuevoBarco->sem == NULL) {
        printf("Error creando semaforo\n");
        free(nuevoBarco);
        return;
    }

    // Configuración de tiempo de cruce y prioridad según el tipo de barco
    if (tipo == 1) {
        nuevoBarco->tiempoCruce = 5;
        nuevoBarco->prioridad = 1;
    } else if (tipo == 2) {
        nuevoBarco->tiempoCruce = 4;
        nuevoBarco->prioridad = 2;
    } else if (tipo == 3) {
        nuevoBarco->tiempoCruce = 2;
        nuevoBarco->prioridad = 3;
    }
    

    // El tiempo restante inicialmente es igual al tiempo de cruce total
    nuevoBarco->tiempoRestante = nuevoBarco->tiempoCruce;
    // Asignamos un deadline aleatorio para el EDF (entre 10 y 30 segundos a partir del momento actual)
    nuevoBarco->deadline = (int)(pdTICKS_TO_MS(xTaskGetTickCount()) / 1000) + (rand() % 20 + 10);

    // Agregar el barco al scheduler
    if (!enqueueBarco(nuevoBarco)) {
        printf("Cola llena\n");
        vSemaphoreDelete(nuevoBarco->sem);
        free(nuevoBarco);
        return;
    }

    printf("Barco %d agregado al scheduler\n", nuevoBarco->id);
    /* Mensaje estructurado para la GUI */
    printf("GUI:ADD,%d,%d,%d,%d,%d,%d\n",
        nuevoBarco->id, nuevoBarco->tipo, nuevoBarco->lado,
        nuevoBarco->prioridad, nuevoBarco->tiempoRestante, nuevoBarco->deadline);

    // Crear la tarea para el barco
    xTaskCreate(
        barcoTask,
        "Barco",
        4096,
        nuevoBarco,
        nuevoBarco->prioridad,
        NULL
    );
}
