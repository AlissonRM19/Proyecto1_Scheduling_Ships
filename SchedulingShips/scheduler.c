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
#include "edf_scheduler.h"
#include "config.h"

// ===== COLAS =====
fcfs_scheduler_t fcfs_Izq, fcfs_Der;
priority_scheduler_t prio_Izq, prio_Der;
sjf_scheduler_t sjf_Izq, sjf_Der;
rr_scheduler_t rr_Izq, rr_Der;
strn_scheduler_t strn_Izq, strn_Der;
edf_scheduler_t edf_Izq, edf_Der;

SemaphoreHandle_t queueMutex;
SemaphoreHandle_t doneSemaphore;

int sentidoActual = 0;
Barco* barco_en_canal = NULL;

// ===== HELPER: hay barcos en un lado? =====
// Evita repetir el bloque if/else de schedulers en cada verificación
static bool hayBarcosEnLado(int lado) {
    if (lado == 0) {
        if (SCHEDULER_ACTUAL == SCHEDULER_FCFS)     return fcfs_Izq.count > 0;
        else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) return prio_Izq.count > 0;
        else if (SCHEDULER_ACTUAL == SCHEDULER_SJF)  return sjf_Izq.count > 0;
        else if (SCHEDULER_ACTUAL == SCHEDULER_RR)   return rr_Izq.count > 0;
        else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) return strn_Izq.count > 0;
        else if (SCHEDULER_ACTUAL == SCHEDULER_EDF)  return edf_Izq.count > 0;
    } else {
        if (SCHEDULER_ACTUAL == SCHEDULER_FCFS)     return fcfs_Der.count > 0;
        else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) return prio_Der.count > 0;
        else if (SCHEDULER_ACTUAL == SCHEDULER_SJF)  return sjf_Der.count > 0;
        else if (SCHEDULER_ACTUAL == SCHEDULER_RR)   return rr_Der.count > 0;
        else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) return strn_Der.count > 0;
        else if (SCHEDULER_ACTUAL == SCHEDULER_EDF)  return edf_Der.count > 0;
    }
    return false;
}

// ===== ENQUEUE =====
bool enqueueBarco(Barco *barco) {
    if (xSemaphoreTake(queueMutex, portMAX_DELAY) != pdTRUE) return false;
    bool result = false;
    if (barco->lado == 0) {
        if (SCHEDULER_ACTUAL == SCHEDULER_FCFS)     result = fcfs_scheduler_enqueue(&fcfs_Izq, barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) result = priority_scheduler_enqueue(&prio_Izq, barco, barco->prioridad);
        else if (SCHEDULER_ACTUAL == SCHEDULER_SJF)  result = sjf_scheduler_enqueue(&sjf_Izq, barco, barco->tiempoCruce);
        else if (SCHEDULER_ACTUAL == SCHEDULER_RR)   result = rr_scheduler_enqueue(&rr_Izq, barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) result = strn_scheduler_enqueue(&strn_Izq, barco, barco->tiempoRestante);
        else if (SCHEDULER_ACTUAL == SCHEDULER_EDF)  result = edf_scheduler_enqueue(&edf_Izq, barco, barco->deadline);
    } else {
        if (SCHEDULER_ACTUAL == SCHEDULER_FCFS)     result = fcfs_scheduler_enqueue(&fcfs_Der, barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) result = priority_scheduler_enqueue(&prio_Der, barco, barco->prioridad);
        else if (SCHEDULER_ACTUAL == SCHEDULER_SJF)  result = sjf_scheduler_enqueue(&sjf_Der, barco, barco->tiempoCruce);
        else if (SCHEDULER_ACTUAL == SCHEDULER_RR)   result = rr_scheduler_enqueue(&rr_Der, barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) result = strn_scheduler_enqueue(&strn_Der, barco, barco->tiempoRestante);
        else if (SCHEDULER_ACTUAL == SCHEDULER_EDF)  result = edf_scheduler_enqueue(&edf_Der, barco, barco->deadline);
    }
    xSemaphoreGive(queueMutex);
    return result;
}

// ===== DEQUEUE =====
Barco* dequeueBarco() {
    Barco *barco = NULL;
    if (xSemaphoreTake(queueMutex, portMAX_DELAY) != pdTRUE) return NULL;
    if (sentidoActual == 0) {
        if (SCHEDULER_ACTUAL == SCHEDULER_FCFS)     fcfs_scheduler_dequeue(&fcfs_Izq, (void**)&barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) priority_scheduler_dequeue(&prio_Izq, (void**)&barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_SJF)  sjf_scheduler_dequeue(&sjf_Izq, (void**)&barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_RR)   rr_scheduler_dequeue(&rr_Izq, (void**)&barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) strn_scheduler_dequeue(&strn_Izq, (void**)&barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_EDF)  edf_scheduler_dequeue(&edf_Izq, (void**)&barco);
    } else {
        if (SCHEDULER_ACTUAL == SCHEDULER_FCFS)     fcfs_scheduler_dequeue(&fcfs_Der, (void**)&barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) priority_scheduler_dequeue(&prio_Der, (void**)&barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_SJF)  sjf_scheduler_dequeue(&sjf_Der, (void**)&barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_RR)   rr_scheduler_dequeue(&rr_Der, (void**)&barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) strn_scheduler_dequeue(&strn_Der, (void**)&barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_EDF)  edf_scheduler_dequeue(&edf_Der, (void**)&barco);
    }
    xSemaphoreGive(queueMutex);
    return barco;
}

// ===== TAREA DEL LETRERO =====
// Cambia el sentido del canal cada tiempoLetrero segundos, independiente de los barcos
void letreroTask(void *pvParameters) {
    while (true) {
        // Esperar el tiempo configurado
        vTaskDelay(pdMS_TO_TICKS(miConfig.tiempoLetrero * 1000));

        // Cambiar sentido
        sentidoActual = (sentidoActual == 0) ? 1 : 0;
        printf("[Letrero] Cambio de sentido -> lado %d (%s)\n",
               sentidoActual,
               sentidoActual == 0 ? "Izquierda" : "Derecha");
    }
}

// ===== INIT =====
void iniciarScheduler() {
    fcfs_scheduler_init(&fcfs_Izq);
    priority_scheduler_init(&prio_Izq);
    sjf_scheduler_init(&sjf_Izq);
    rr_scheduler_init(&rr_Izq);
    strn_scheduler_init(&strn_Izq);
    edf_scheduler_init(&edf_Izq);

    fcfs_scheduler_init(&fcfs_Der);
    priority_scheduler_init(&prio_Der);
    sjf_scheduler_init(&sjf_Der);
    rr_scheduler_init(&rr_Der);
    strn_scheduler_init(&strn_Der);
    edf_scheduler_init(&edf_Der);

    queueMutex = xSemaphoreCreateMutex();
    doneSemaphore = xSemaphoreCreateBinary();


        // Crear tarea del letrero solo si el modo lo requiere
    if (miConfig.modoControl == MODO_LETRERO) {
        xTaskCreate(
            letreroTask,
            "Letrero",
            2048,
            NULL,
            2,      // Prioridad menor que schedulerTask (3)
            NULL
        );
    }

    
    // Creamos la tarea del scheduler
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

    int barcosPasados = 0; // Para MODO_EQUIDAD: barcos cruzados consecutivamente en sentidoActual

    while (true) {

        // =========================
        // MODO TICO: elegir lado dinámicamente — el primero que tenga barcos
        // =========================
        if (miConfig.modoControl == MODO_TICO) {
            bool hayIzq = hayBarcosEnLado(0);
            bool hayDer = hayBarcosEnLado(1);

            if (!hayIzq && !hayDer) {
                // Ningún lado tiene barcos, esperar
                vTaskDelay(pdMS_TO_TICKS(200));
                continue;
            }

            // Tomar el lado que tenga barcos; si ambos tienen, mantener el sentido actual
            // para no cambiar innecesariamente mientras el canal está libre
            if (!hayBarcosEnLado(sentidoActual)) {
                sentidoActual = (sentidoActual == 0) ? 1 : 0;
                printf("[Tico] Sin barcos en lado actual, cambiando a lado %d (%s)\n",
                       sentidoActual, sentidoActual == 0 ? "Izquierda" : "Derecha");
            }

            // Despachar el próximo barco del lado actual
            Barco *proximo = dequeueBarco();
            if (proximo == NULL) continue;

            if (SCHEDULER_ACTUAL == SCHEDULER_FCFS)     printf("FCFS -> turno para barco %d\n", proximo->id);
            else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) printf("PRIORITY -> turno para barco %d | prioridad %d\n", proximo->id, proximo->prioridad);
            else if (SCHEDULER_ACTUAL == SCHEDULER_SJF)  printf("SJF -> turno para barco %d | tiempo de cruce %d\n", proximo->id, proximo->tiempoCruce);
            else if (SCHEDULER_ACTUAL == SCHEDULER_RR)   printf("RR -> turno para barco %d | tiempo restante %ds | quantum %ds\n", proximo->id, proximo->tiempoRestante, RR_QUANTUM);
            else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) printf("STRN -> turno para barco %d | tiempo restante %ds\n", proximo->id, proximo->tiempoRestante);
            else if (SCHEDULER_ACTUAL == SCHEDULER_EDF)  printf("EDF -> turno para barco %d | deadline %d\n", proximo->id, proximo->deadline);

            barco_en_canal = proximo;
            xSemaphoreGive(proximo->sem);
            xSemaphoreTake(doneSemaphore, portMAX_DELAY);
            barco_en_canal = NULL;

            if ((SCHEDULER_ACTUAL == SCHEDULER_RR || SCHEDULER_ACTUAL == SCHEDULER_STRN) && proximo->tiempoRestante > 0) {
                printf("%s -> barco %d regresa a la cola con tiempo restante\n",
                       (SCHEDULER_ACTUAL == SCHEDULER_RR ? "RR" : "STRN"), proximo->id);
                enqueueBarco(proximo);
            } else {
                printf("Barco %d terminó y libera el canal\n", proximo->id);
            }
            continue; // Volver al inicio del loop (modo Tico no usa el resto)
        }

        // =========================
        // MODO EQUIDAD / LETRERO: verificar si hay barcos en el lado actual
        // =========================
        bool colaVacia = !hayBarcosEnLado(sentidoActual);

        if (colaVacia) {
            // Cola actual vacía: si hay barcos al otro lado, cambiar sentido
            int otroLado = (sentidoActual == 0) ? 1 : 0;
            if (hayBarcosEnLado(otroLado)) {
                sentidoActual = otroLado;
                barcosPasados = 0;
                printf("[%s] Cola vacía en lado %d, cambiando a lado %d (%s)\n",
                       miConfig.modoControl == MODO_EQUIDAD ? "Equidad" : "Letrero",
                       (sentidoActual == 0 ? 1 : 0), sentidoActual,
                       sentidoActual == 0 ? "Izquierda" : "Derecha");
            }
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        // =========================
        // MODO EQUIDAD: chequear límite W antes de despachar
        // =========================
        if (miConfig.modoControl == MODO_EQUIDAD) {
            int otroLado = (sentidoActual == 0) ? 1 : 0;
            if (barcosPasados >= miConfig.parametroW && hayBarcosEnLado(otroLado)) {
                sentidoActual = otroLado;
                barcosPasados = 0;
                printf("[Equidad] W=%d alcanzado, cambiando a lado %d (%s)\n",
                       miConfig.parametroW, sentidoActual,
                       sentidoActual == 0 ? "Izquierda" : "Derecha");
                continue;
            }
        }

        // =========================
        // Sacar siguiente barco
        // =========================
        Barco *proximo = dequeueBarco();
        if (proximo == NULL) continue;

        if (SCHEDULER_ACTUAL == SCHEDULER_FCFS)     printf("FCFS -> turno para barco %d\n", proximo->id);
        else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) printf("PRIORITY -> turno para barco %d | prioridad %d\n", proximo->id, proximo->prioridad);
        else if (SCHEDULER_ACTUAL == SCHEDULER_SJF)  printf("SJF -> turno para barco %d | tiempo de cruce %d\n", proximo->id, proximo->tiempoCruce);
        else if (SCHEDULER_ACTUAL == SCHEDULER_RR)   printf("RR -> turno para barco %d | tiempo restante %ds | quantum %ds\n", proximo->id, proximo->tiempoRestante, RR_QUANTUM);
        else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) printf("STRN -> turno para barco %d | tiempo restante %ds\n", proximo->id, proximo->tiempoRestante);
        else if (SCHEDULER_ACTUAL == SCHEDULER_EDF)  printf("EDF -> turno para barco %d | deadline %d\n", proximo->id, proximo->deadline);

        barco_en_canal = proximo;
        xSemaphoreGive(proximo->sem);
        xSemaphoreTake(doneSemaphore, portMAX_DELAY);
        barco_en_canal = NULL;

        if ((SCHEDULER_ACTUAL == SCHEDULER_RR || SCHEDULER_ACTUAL == SCHEDULER_STRN) && proximo->tiempoRestante > 0) {
            printf("%s -> barco %d regresa a la cola con tiempo restante\n",
                   (SCHEDULER_ACTUAL == SCHEDULER_RR ? "RR" : "STRN"), proximo->id);
            enqueueBarco(proximo);
        } else {
            printf("Barco %d terminó y libera el canal\n", proximo->id);
            barcosPasados++;
        }
    }
}