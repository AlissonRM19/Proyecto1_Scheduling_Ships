#include <stdio.h>      // Para printf
#include <stdlib.h>     // Para malloc y free
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "barcos.h"
#include "config.h"
#include "scheduler.h"

int contadorBarcos = 1;

const char* nombreTipo(int tipo) {
    if (tipo == 1) return "Normal";
    if (tipo == 2) return "Pesquero";
    if (tipo == 3) return "Patrulla";
    return "Desconocido";
}

const char* nombreLado(int lado) {
    if (lado == 0) return "Izquierda";
    return "Derecha";
}

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

    while (barco->tiempoRestante > 0) {
        xSemaphoreTake(barco->sem, portMAX_DELAY);

        int slice = barco->tiempoRestante;

        if (SCHEDULER_ACTUAL == SCHEDULER_RR && slice > RR_QUANTUM) {
            slice = RR_QUANTUM;
        }

        printf("Barco %d comienza a cruzar el canal...\n", barco->id);

        vTaskDelay(slice * 1000 / portTICK_PERIOD_MS);

        barco->tiempoRestante -= slice;

        if (barco->tiempoRestante > 0) {
            printf("Barco %d pausa, quedan %ds\n", barco->id, barco->tiempoRestante);
        } else {
            printf("Barco %d completó el cruce!\n", barco->id);
        }

        xSemaphoreGive(doneSemaphore);
    }

    vSemaphoreDelete(barco->sem);
    free(barco);
    vTaskDelete(NULL);
}

// ===== CREAR BARCO =====

void crearBarco(int tipo, int lado) {
    Barco *nuevoBarco = (Barco*) malloc(sizeof(Barco));

    if (nuevoBarco == NULL) {
        printf("Error reservando memoria\n");
        return;
    }

    nuevoBarco->id = contadorBarcos++;
    nuevoBarco->tipo = tipo;
    nuevoBarco->lado = lado;

    nuevoBarco->sem = xSemaphoreCreateBinary();

    if (nuevoBarco->sem == NULL) {
        printf("Error creando semaforo\n");
        free(nuevoBarco);
        return;
    }

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

    nuevoBarco->tiempoRestante = nuevoBarco->tiempoCruce;

    nuevoBarco->deadline = (int)(pdTICKS_TO_MS(xTaskGetTickCount()) / 1000) + (rand() % 20 + 10);


    if (!enqueueBarco(nuevoBarco)) {
        printf("Cola llena\n");
        vSemaphoreDelete(nuevoBarco->sem);
        free(nuevoBarco);
        return;
    }

    printf("Barco %d agregado al scheduler\n", nuevoBarco->id);

    xTaskCreate(
        barcoTask,
        "Barco",
        4096,
        nuevoBarco,
        nuevoBarco->prioridad,
        NULL
    );
}
