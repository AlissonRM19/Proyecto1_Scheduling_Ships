#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "scheduler.h"
#include "barcos.h"
#include "fcfs_scheduler.h"
#include "priority_scheduler.h"
#include "sjf_scheduler.h"
#include "rr_scheduler.h"
#include "strn_scheduler.h"
#include "config.h"

// ===== COLAS =====
fcfs_scheduler_t fcfsQueue;
priority_scheduler_t priorityQueue;
sjf_scheduler_t sjfQueue;
rr_scheduler_t rrQueue;
strn_scheduler_t strnQueue;

SemaphoreHandle_t queueMutex;
SemaphoreHandle_t doneSemaphore;

// ===== ENQUEUE =====

bool enqueueBarco(Barco *barco) {
    if (xSemaphoreTake(queueMutex, portMAX_DELAY) != pdTRUE) {
        return false;
    }

    bool result = false;

    if (SCHEDULER_ACTUAL == SCHEDULER_FCFS) {
        result = fcfs_scheduler_enqueue(&fcfsQueue, barco);
    }
    else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) {
        result = priority_scheduler_enqueue(&priorityQueue, barco, barco->prioridad);
    }
    else if (SCHEDULER_ACTUAL == SCHEDULER_SJF) {
        result = sjf_scheduler_enqueue(&sjfQueue, barco, barco->tiempoCruce);
    }
    else if (SCHEDULER_ACTUAL == SCHEDULER_RR) {
        result = rr_scheduler_enqueue(&rrQueue, barco);
    }
    else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) {
        result = strn_scheduler_enqueue(&strnQueue, barco, barco->tiempoRestante);
    }

    xSemaphoreGive(queueMutex);
    return result;
}

// ===== DEQUEUE =====

Barco* dequeueBarco() {
    Barco *barco = NULL;

    if (xSemaphoreTake(queueMutex, portMAX_DELAY) != pdTRUE) {
        return NULL;
    }

    if (SCHEDULER_ACTUAL == SCHEDULER_FCFS) {
        if (!fcfs_scheduler_dequeue(&fcfsQueue, (void**)&barco)) barco = NULL;
    }
    else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) {
        if (!priority_scheduler_dequeue(&priorityQueue, (void**)&barco)) barco = NULL;
    }
    else if (SCHEDULER_ACTUAL == SCHEDULER_SJF) {
        if (!sjf_scheduler_dequeue(&sjfQueue, (void**)&barco)) barco = NULL;
    }
    else if (SCHEDULER_ACTUAL == SCHEDULER_RR) {
        if (!rr_scheduler_dequeue(&rrQueue, (void**)&barco)) barco = NULL;
    }
    else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) {
        if (!strn_scheduler_dequeue(&strnQueue, (void**)&barco)) barco = NULL;
    }

    xSemaphoreGive(queueMutex);
    return barco;
}

// ===== INIT =====

void iniciarScheduler() {
    fcfs_scheduler_init(&fcfsQueue);
    priority_scheduler_init(&priorityQueue);
    sjf_scheduler_init(&sjfQueue);
    rr_scheduler_init(&rrQueue);
    strn_scheduler_init(&strnQueue);

    queueMutex = xSemaphoreCreateMutex();
    doneSemaphore = xSemaphoreCreateBinary();

    xTaskCreate(
        schedulerTask,
        "Scheduler",
        4096,
        NULL,
        3,
        NULL
    );
}

// ===== TASK DEL SCHEDULER =====

void schedulerTask(void *pvParameters) {
    while (true) {
        // =========================
        // Verificar si hay barcos
        // =========================
        bool colaVacia = true;

        if (SCHEDULER_ACTUAL == SCHEDULER_FCFS) colaVacia = (fcfsQueue.count == 0);
        else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) colaVacia = (priorityQueue.count == 0);
        else if (SCHEDULER_ACTUAL == SCHEDULER_SJF) colaVacia = (sjfQueue.count == 0);
        else if (SCHEDULER_ACTUAL == SCHEDULER_RR) colaVacia = (rrQueue.count == 0);
        else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) colaVacia = (strnQueue.count == 0);

        if (colaVacia) {
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        // =========================
        // Sacar siguiente barco
        // =========================
        Barco *proximo = dequeueBarco();
        if (proximo == NULL) continue;

        // =========================
        // Mostrar scheduler activo (Uso de printf)
        // =========================
        if (SCHEDULER_ACTUAL == SCHEDULER_FCFS) {
            printf("FCFS -> turno para barco %d\n", proximo->id);
        }
        else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) {
            printf("PRIORITY -> turno para barco %d | prioridad %d\n", proximo->id, proximo->prioridad);
        }
        else if (SCHEDULER_ACTUAL == SCHEDULER_SJF) {
            printf("SJF -> turno para barco %d | tiempo de cruce %d\n", proximo->id, proximo->tiempoCruce);
        }
        else if (SCHEDULER_ACTUAL == SCHEDULER_RR) {
            printf("RR -> turno para barco %d | tiempo restante %ds | quantum %ds\n", proximo->id, proximo->tiempoRestante, RR_QUANTUM);
        }
        else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) {
            printf("STRN -> turno para barco %d | tiempo restante %ds\n", proximo->id, proximo->tiempoRestante);
        }

        // =========================
        // Autorizar cruce
        // =========================
        xSemaphoreGive(proximo->sem);

        // =========================
        // Esperar que termine
        // =========================
        xSemaphoreTake(doneSemaphore, portMAX_DELAY);

        // Re-encolar si es necesario (Preemptive/Quantum)
        if ((SCHEDULER_ACTUAL == SCHEDULER_RR || SCHEDULER_ACTUAL == SCHEDULER_STRN) && proximo->tiempoRestante > 0) {
            printf("%s -> barco %d regresa a la cola con tiempo restante\n", 
                   (SCHEDULER_ACTUAL == SCHEDULER_RR ? "RR" : "STRN"), proximo->id);
            enqueueBarco(proximo);
        } else {
            printf("Barco %d terminó y libera el canal\n", proximo->id);
        }
    }
}