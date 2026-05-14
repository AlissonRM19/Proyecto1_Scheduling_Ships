#include <stdio.h>      // Para printf
#include <stdlib.h>     // Para malloc y free
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "barcos.h"
#include "hardware.h"
#include "config.h"
#include "scheduler.h"

int contadorBarcos = 1;

extern int barcos_en_canal_count;
extern int sentido_actual;
extern SemaphoreHandle_t flujoMutex;

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

    printf("=================================\n");
    printf("Barco creado -> ID: %d\n", barco->id);
    printf("Tipo: %s\n", nombreTipo(barco->tipo));
    printf("Lado de origen: %s\n", nombreLado(barco->lado));
    printf("Tiempo de cruce: %d\n", barco->tiempoCruce);
    printf("Prioridad: %d\n", barco->prioridad);
    printf("Estado: En cola de listos\n");
    printf("=================================\n");

    // El barco espera a ser programado y luego comienza a cruzar
    while (barco->tiempoRestante > 0) {
        // 1. Esperamos permiso del Scheduler (Turno de CPU)
        xSemaphoreTake(barco->sem, portMAX_DELAY);

        // --- PROTOCOLO DE ENTRADA AL CANAL (RECURSO CRÍTICO) ---
        bool puedeEntrar = false;
        while (!puedeEntrar) {
            xSemaphoreTake(flujoMutex, portMAX_DELAY);
            
            // Si el canal está vacío, el primero que llega establece el sentido
            if (barcos_en_canal_count == 0) {
                sentido_actual = barco->lado;
                puedeEntrar = true;
            } 
            // Si hay barcos, solo entra si van en su misma dirección
            else if (sentido_actual == barco->lado) {
                puedeEntrar = true;
            }
            
            if (puedeEntrar) {
                barcos_en_canal_count++;
            }
            
            xSemaphoreGive(flujoMutex);

            if (!puedeEntrar) {
                // Si el canal está bloqueado por el sentido opuesto, 
                // esperamos un poco para no saturar el procesador
                vTaskDelay(pdMS_TO_TICKS(50));
            }
        }

        // --- INICIO DEL CRUCE ---
        int slice = barco->tiempoRestante;
        if (SCHEDULER_ACTUAL == SCHEDULER_RR && slice > RR_QUANTUM) {
            slice = RR_QUANTUM;
        }

        registrarBarcoEnCanal(barco);
        printf("Barco %d entrando al canal (Sentido: %d)\n", barco->id, sentido_actual);

        int delayMovimiento = (miConfig.velocidadBarco / 5); 

        if (barco->lado == 0) { // IZQ -> DER
            for (int p = 4; p <= 8; p++) {
                barco->posicion = p;
                actualizar_tira_led();
                vTaskDelay(pdMS_TO_TICKS(delayMovimiento)); 
            }
        } else { // DER -> IZQ
            for (int p = 8; p >= 4; p--) {
                barco->posicion = p;
                actualizar_tira_led();
                vTaskDelay(pdMS_TO_TICKS(delayMovimiento));
            }
        }

        // --- PROTOCOLO DE SALIDA DEL CANAL ---
        desregistrarBarcoEnCanal(barco);
        
        xSemaphoreTake(flujoMutex, portMAX_DELAY);
        barcos_en_canal_count--;
        if (barcos_en_canal_count == 0) {
            sentido_actual = -1; // Canal totalmente libre
        }
        xSemaphoreGive(flujoMutex);

        // --- LÓGICA DE TIEMPO RESTANTE ---
        if (barco->tiempoRestante <= slice) { 
            barco->tiempoRestante = 0;
            printf("Barco %d completó el cruce!\n", barco->id);
        } else {
            barco->tiempoRestante -= slice;
            printf("Barco %d pausa (Quantum), restan %ds\n", barco->id, barco->tiempoRestante);
        }

        xSemaphoreGive(sem_espacio_canal);
        actualizar_tira_led(); 

        // 4. Avisamos al scheduler que nuestra ráfaga terminó
        xSemaphoreGive(doneSemaphore);
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
    nuevoBarco->posicion = (lado == 0) ? 0 : 11; // Espera en los extremos

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
