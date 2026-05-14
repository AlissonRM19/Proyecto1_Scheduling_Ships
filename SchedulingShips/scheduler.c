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
SemaphoreHandle_t sem_espacio_canal; // El "controlador" del pipeline

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

    // Esto ayuda a detectar deadlocks más fácilmente
    if (xSemaphoreTake(queueMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return NULL; 
    }

    // Seleccionamos la cola según el sentido actual
    if (sentidoActual == 0) {
        switch (SCHEDULER_ACTUAL) {
            case SCHEDULER_FCFS:     fcfs_scheduler_dequeue(&fcfs_Izq, (void**)&barco); break;
            case SCHEDULER_PRIORITY: priority_scheduler_dequeue(&prio_Izq, (void**)&barco); break;
            case SCHEDULER_SJF:      sjf_scheduler_dequeue(&sjf_Izq, (void**)&barco); break;
            case SCHEDULER_RR:       rr_scheduler_dequeue(&rr_Izq, (void**)&barco); break;
            case SCHEDULER_STRN:     strn_scheduler_dequeue(&strn_Izq, (void**)&barco); break;
            case SCHEDULER_EDF:      edf_scheduler_dequeue(&edf_Izq, (void**)&barco); break;
        }
    } else {
        switch (SCHEDULER_ACTUAL) {
            case SCHEDULER_FCFS:     fcfs_scheduler_dequeue(&fcfs_Der, (void**)&barco); break;
            case SCHEDULER_PRIORITY: priority_scheduler_dequeue(&prio_Der, (void**)&barco); break;
            case SCHEDULER_SJF:      sjf_scheduler_dequeue(&sjf_Der, (void**)&barco); break;
            case SCHEDULER_RR:       rr_scheduler_dequeue(&rr_Der, (void**)&barco); break;
            case SCHEDULER_STRN:     strn_scheduler_dequeue(&strn_Der, (void**)&barco); break;
            case SCHEDULER_EDF:      edf_scheduler_dequeue(&edf_Der, (void**)&barco); break;
        }
    }

    xSemaphoreGive(queueMutex);

    // Si barco sigue siendo NULL, significa que la cola seleccionada estaba vacía
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
        printf("GUI:DIR,%d\n", sentidoActual);
        printf("[Letrero] Cambio de sentido -> lado %d (%s)\n",
               sentidoActual,
               sentidoActual == 0 ? "Izquierda" : "Derecha");
    }
}

// ===== INIT =====
void iniciarScheduler() {
    /* Estado inicial para la GUI */
    printf("GUI:SCH,%d\n", SCHEDULER_ACTUAL);
    printf("GUI:MODO,%d\n", miConfig.modoControl);
    printf("GUI:DIR,%d\n", sentidoActual);

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
    sem_espacio_canal = xSemaphoreCreateCounting(5, 5); // Máximo 5 barcos


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
    printf("[Scheduler] Tarea iniciada.\n");
    int barcosPasados = 0;

    while (1) {
        // 1. Verificar si hay barcos en algún lado antes de proceder
        bool hayIzq = hayBarcosEnLado(0);
        bool hayDer = hayBarcosEnLado(1);

        if (!hayIzq && !hayDer) {
            // Si no hay barcos, esperamos un poco y volvemos a chequear.
            // Esto evita que el CPU se sature y previene errores de acceso.
            vTaskDelay(pdMS_TO_TICKS(100));
            continue; 
        }

        // 2. Lógica de cambio de sentido (Simplificada para estabilidad)
        if (!hayBarcosEnLado(sentidoActual)) {
            sentidoActual = (sentidoActual == 0) ? 1 : 0;
            printf("GUI:DIR,%d\n", sentidoActual);
            vTaskDelay(pdMS_TO_TICKS(50)); // Pequeño respiro para la GUI
        }

        // 3. EXTRACCIÓN SEGURA (Aquí es donde ocurría el Load Access Fault)
        Barco *proximo = dequeueBarco();

        if (proximo == NULL) {
            // Si por alguna razón llegó aquí y es NULL, abortamos este ciclo.
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        // 4. Procesamiento del barco (Solo si proximo != NULL)
        barco_en_canal = proximo;
        
        int guiSlice = proximo->tiempoRestante;
        // Ajuste para Round Robin si aplica
        if (SCHEDULER_ACTUAL == 2 && guiSlice > 2) { // Asumiendo RR y Quantum de 2
            guiSlice = 2;
        }

        printf("GUI:CROSS,%d,%d,%d,%d,%d,%d\n", 
               proximo->id, proximo->tipo, proximo->lado, 
               guiSlice, proximo->tiempoCruce, proximo->tiempoRestante);

        // 5. Sincronización con la tarea del barco
        if (proximo->sem != NULL) {
            xSemaphoreGive(proximo->sem);
            
            // Esperamos a que el barco nos avise que terminó de cruzar
            // Asegúrate de que doneSemaphore esté inicializado en el app_main
            if (xSemaphoreTake(doneSemaphore, pdMS_TO_TICKS(10000)) == pdFALSE) {
                printf("[Error] Timeout esperando al barco %d\n", proximo->id);
            }
        }

        barco_en_canal = NULL;

        // 6. Re-encolar o finalizar
        if (proximo->tiempoRestante > 0) {
            printf("GUI:RETURN,%d,%d\n", proximo->id, proximo->tiempoRestante);
            enqueueBarco(proximo);
        } else {
            printf("GUI:DONE,%d\n", proximo->id);
            barcosPasados++;
        }
        
        vTaskDelay(pdMS_TO_TICKS(50)); // Delay de cortesía entre barcos
    }
}
