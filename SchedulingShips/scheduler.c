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

// Por ahora fijo en 0 (Izquierda). Esto lo moverá el "Letrero" o "Equidad".
int sentidoActual = 0;
Barco* barco_en_canal = NULL;

// ===== ENQUEUE =====
// Encola un barco en la cola correspondiente al scheduler activo, usando su prioridad, tiempo de cruce, tiempo restante o deadline según corresponda
bool enqueueBarco(Barco *barco) {

    
    if (xSemaphoreTake(queueMutex, portMAX_DELAY) != pdTRUE) {
        return false;
    }

    bool result = false;

    if (barco->lado == 0) { // LADO IZQUIERDO
        if (SCHEDULER_ACTUAL == SCHEDULER_FCFS) result = fcfs_scheduler_enqueue(&fcfs_Izq, barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) result = priority_scheduler_enqueue(&prio_Izq, barco, barco->prioridad);
        else if (SCHEDULER_ACTUAL == SCHEDULER_SJF) result = sjf_scheduler_enqueue(&sjf_Izq, barco, barco->tiempoCruce);
        else if (SCHEDULER_ACTUAL == SCHEDULER_RR) result = rr_scheduler_enqueue(&rr_Izq, barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) result = strn_scheduler_enqueue(&strn_Izq, barco, barco->tiempoRestante);
        else if (SCHEDULER_ACTUAL == SCHEDULER_EDF) result = edf_scheduler_enqueue(&edf_Izq, barco, barco->deadline);
    } 
    else { // LADO DERECHO
        if (SCHEDULER_ACTUAL == SCHEDULER_FCFS) result = fcfs_scheduler_enqueue(&fcfs_Der, barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) result = priority_scheduler_enqueue(&prio_Der, barco, barco->prioridad);
        else if (SCHEDULER_ACTUAL == SCHEDULER_SJF) result = sjf_scheduler_enqueue(&sjf_Der, barco, barco->tiempoCruce);
        else if (SCHEDULER_ACTUAL == SCHEDULER_RR) result = rr_scheduler_enqueue(&rr_Der, barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) result = strn_scheduler_enqueue(&strn_Der, barco, barco->tiempoRestante);
        else if (SCHEDULER_ACTUAL == SCHEDULER_EDF) result = edf_scheduler_enqueue(&edf_Der, barco, barco->deadline);
    }
    
    xSemaphoreGive(queueMutex);
    return result;
}

// ===== DEQUEUE =====
// Devuelve el siguiente barco a autorizar según el scheduler activo, o NULL si no hay barcos
// El caller debe verificar si el resultado es NULL antes de usarlo
Barco* dequeueBarco() {
    Barco *barco = NULL;

    if (xSemaphoreTake(queueMutex, portMAX_DELAY) != pdTRUE) {
        return NULL;
    }

    // Aquí el scheduler solo saca barcos del lado que tiene el permiso (sentidoActual)
    if (sentidoActual == 0) { // IZQUIERDA
        if (SCHEDULER_ACTUAL == SCHEDULER_FCFS) fcfs_scheduler_dequeue(&fcfs_Izq, (void**)&barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) priority_scheduler_dequeue(&prio_Izq, (void**)&barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_SJF) sjf_scheduler_dequeue(&sjf_Izq, (void**)&barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_RR) rr_scheduler_dequeue(&rr_Izq, (void**)&barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) strn_scheduler_dequeue(&strn_Izq, (void**)&barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_EDF) edf_scheduler_dequeue(&edf_Izq, (void**)&barco);
    } 
    else { // DERECHA
        if (SCHEDULER_ACTUAL == SCHEDULER_FCFS) fcfs_scheduler_dequeue(&fcfs_Der, (void**)&barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) priority_scheduler_dequeue(&prio_Der, (void**)&barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_SJF) sjf_scheduler_dequeue(&sjf_Der, (void**)&barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_RR) rr_scheduler_dequeue(&rr_Der, (void**)&barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) strn_scheduler_dequeue(&strn_Der, (void**)&barco);
        else if (SCHEDULER_ACTUAL == SCHEDULER_EDF) edf_scheduler_dequeue(&edf_Der, (void**)&barco);
    }

    xSemaphoreGive(queueMutex);
    return barco;
}

// ===== INIT =====
// Inicializa las colas y crea la tarea del scheduler
void iniciarScheduler() {
  
    // Inicializar IZQUIERDA
    fcfs_scheduler_init(&fcfs_Izq);
    priority_scheduler_init(&prio_Izq);
    sjf_scheduler_init(&sjf_Izq);
    rr_scheduler_init(&rr_Izq);
    strn_scheduler_init(&strn_Izq);
    edf_scheduler_init(&edf_Izq);

    // Inicializar DERECHA
    fcfs_scheduler_init(&fcfs_Der);
    priority_scheduler_init(&prio_Der);
    sjf_scheduler_init(&sjf_Der);
    rr_scheduler_init(&rr_Der);
    strn_scheduler_init(&strn_Der);
    edf_scheduler_init(&edf_Der);

    // Inicializamos los semáforos
    // El mutex protege el acceso a las colas, y el semáforo "done" se usa para sincronizar con los barcos
    queueMutex = xSemaphoreCreateMutex();
    doneSemaphore = xSemaphoreCreateBinary();

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
// Esta tarea corre en un loop infinito, verificando si hay barcos en la cola, autorizando su cruce, y esperando a que terminen

void schedulerTask(void *pvParameters) {

    // --- Estado para MODO_EQUIDAD ---
    int barcosPasados = 0; // Cuántos barcos han cruzado consecutivamente en el sentido actual

    while (true) {
        // =========================
        // Verificar si hay barcos en el lado actual
        // =========================
        bool colaVacia = true;

        if (sentidoActual == 0) {
            if (SCHEDULER_ACTUAL == SCHEDULER_FCFS)     colaVacia = (fcfs_Izq.count == 0);
            else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) colaVacia = (prio_Izq.count == 0);
            else if (SCHEDULER_ACTUAL == SCHEDULER_SJF)  colaVacia = (sjf_Izq.count == 0);
            else if (SCHEDULER_ACTUAL == SCHEDULER_RR)   colaVacia = (rr_Izq.count == 0);
            else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) colaVacia = (strn_Izq.count == 0);
            else if (SCHEDULER_ACTUAL == SCHEDULER_EDF)  colaVacia = (edf_Izq.count == 0);
        } else {
            if (SCHEDULER_ACTUAL == SCHEDULER_FCFS)     colaVacia = (fcfs_Der.count == 0);
            else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) colaVacia = (prio_Der.count == 0);
            else if (SCHEDULER_ACTUAL == SCHEDULER_SJF)  colaVacia = (sjf_Der.count == 0);
            else if (SCHEDULER_ACTUAL == SCHEDULER_RR)   colaVacia = (rr_Der.count == 0);
            else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) colaVacia = (strn_Der.count == 0);
            else if (SCHEDULER_ACTUAL == SCHEDULER_EDF)  colaVacia = (edf_Der.count == 0);
        }

        if (colaVacia) {
            // Cola actual vacía: si hay barcos al otro lado, cambiar sentido
            int otroLado = (sentidoActual == 0) ? 1 : 0;
            bool otroTieneBarcos = false;

            if (otroLado == 0) {
                if (SCHEDULER_ACTUAL == SCHEDULER_FCFS)     otroTieneBarcos = (fcfs_Izq.count > 0);
                else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) otroTieneBarcos = (prio_Izq.count > 0);
                else if (SCHEDULER_ACTUAL == SCHEDULER_SJF)  otroTieneBarcos = (sjf_Izq.count > 0);
                else if (SCHEDULER_ACTUAL == SCHEDULER_RR)   otroTieneBarcos = (rr_Izq.count > 0);
                else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) otroTieneBarcos = (strn_Izq.count > 0);
                else if (SCHEDULER_ACTUAL == SCHEDULER_EDF)  otroTieneBarcos = (edf_Izq.count > 0);
            } else {
                if (SCHEDULER_ACTUAL == SCHEDULER_FCFS)     otroTieneBarcos = (fcfs_Der.count > 0);
                else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) otroTieneBarcos = (prio_Der.count > 0);
                else if (SCHEDULER_ACTUAL == SCHEDULER_SJF)  otroTieneBarcos = (sjf_Der.count > 0);
                else if (SCHEDULER_ACTUAL == SCHEDULER_RR)   otroTieneBarcos = (rr_Der.count > 0);
                else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) otroTieneBarcos = (strn_Der.count > 0);
                else if (SCHEDULER_ACTUAL == SCHEDULER_EDF)  otroTieneBarcos = (edf_Der.count > 0);
            }

            if (otroTieneBarcos) {
                sentidoActual = otroLado;
                barcosPasados = 0;
                printf("[Equidad] Cola vacía en lado %d, cambiando a lado %d\n",
                       (sentidoActual == 0 ? 1 : 0), sentidoActual);
            }

            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        // =========================
        // MODO EQUIDAD: chequear límite W antes de despachar
        // =========================
        if (miConfig.modoControl == MODO_EQUIDAD) {
            int otroLado = (sentidoActual == 0) ? 1 : 0;
            bool otroTieneBarcos = false;

            if (otroLado == 0) {
                if (SCHEDULER_ACTUAL == SCHEDULER_FCFS)     otroTieneBarcos = (fcfs_Izq.count > 0);
                else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) otroTieneBarcos = (prio_Izq.count > 0);
                else if (SCHEDULER_ACTUAL == SCHEDULER_SJF)  otroTieneBarcos = (sjf_Izq.count > 0);
                else if (SCHEDULER_ACTUAL == SCHEDULER_RR)   otroTieneBarcos = (rr_Izq.count > 0);
                else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) otroTieneBarcos = (strn_Izq.count > 0);
                else if (SCHEDULER_ACTUAL == SCHEDULER_EDF)  otroTieneBarcos = (edf_Izq.count > 0);
            } else {
                if (SCHEDULER_ACTUAL == SCHEDULER_FCFS)     otroTieneBarcos = (fcfs_Der.count > 0);
                else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) otroTieneBarcos = (prio_Der.count > 0);
                else if (SCHEDULER_ACTUAL == SCHEDULER_SJF)  otroTieneBarcos = (sjf_Der.count > 0);
                else if (SCHEDULER_ACTUAL == SCHEDULER_RR)   otroTieneBarcos = (rr_Der.count > 0);
                else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) otroTieneBarcos = (strn_Der.count > 0);
                else if (SCHEDULER_ACTUAL == SCHEDULER_EDF)  otroTieneBarcos = (edf_Der.count > 0);
            }

            if (barcosPasados >= miConfig.parametroW && otroTieneBarcos) {
                sentidoActual = otroLado;
                barcosPasados = 0;
                printf("[Equidad] W=%d alcanzado, cambiando a lado %d\n",
                       miConfig.parametroW, sentidoActual);
                continue; // Volver al inicio con el nuevo sentido
            }
        }

        // =========================
        // Sacar siguiente barco
        // =========================
        Barco *proximo = dequeueBarco();
        if (proximo == NULL) continue;

        // =========================
        // Mostrar scheduler activo
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
        else if (SCHEDULER_ACTUAL == SCHEDULER_EDF) {
            printf("EDF -> turno para barco %d | deadline %d\n", proximo->id, proximo->deadline);
        }

        // =========================
        // Autorizar cruce
        // =========================
        barco_en_canal = proximo;
        xSemaphoreGive(proximo->sem);

        // =========================
        // Esperar que termine
        // =========================
        xSemaphoreTake(doneSemaphore, portMAX_DELAY);

        barco_en_canal = NULL;

        // Re-encolar si es necesario (RR / STRN)
        if ((SCHEDULER_ACTUAL == SCHEDULER_RR || SCHEDULER_ACTUAL == SCHEDULER_STRN) && proximo->tiempoRestante > 0) {
            printf("%s -> barco %d regresa a la cola con tiempo restante\n",
                   (SCHEDULER_ACTUAL == SCHEDULER_RR ? "RR" : "STRN"), proximo->id);
            enqueueBarco(proximo);
            // No incrementar barcosPasados: el barco no completó su cruce todavía
        } else {
            printf("Barco %d terminó y libera el canal\n", proximo->id);
            barcosPasados++; // Solo cuenta cuando el barco cruzó completamente
        }
    }
}
