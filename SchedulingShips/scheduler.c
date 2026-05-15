#include <stdio.h>  // Librería estándar para entrada/salida
#include <stdbool.h>  // Incluir bool
#include "freertos/FreeRTOS.h"  // Incluir FreeRTOS
#include "freertos/task.h"  // Incluir tareas
#include "freertos/semphr.h"  // Incluir semáforos
#include "freertos/queue.h"  // Incluir colas
#include "scheduler.h"  // Incluir scheduler
#include "barcos.h"  // Incluir barcos
#include "canal.h"  // Incluir canal
#include "fcfs_scheduler.h"  // Incluir FCFS
#include "priority_scheduler.h"  // Incluir Priority
#include "sjf_scheduler.h"  // Incluir SJF
#include "rr_scheduler.h"  // Incluir RR
#include "strn_scheduler.h"  // Incluir STRN
#include "edf_scheduler.h"  // Incluir EDF
#include "config.h"  // Incluir configuración

// Colas de schedulers para cada lado
fcfs_scheduler_t     fcfs_Izq,  fcfs_Der;  // Colas FCFS izquierda y derecha
priority_scheduler_t prio_Izq,  prio_Der;  // Colas Priority izquierda y derecha
sjf_scheduler_t      sjf_Izq,   sjf_Der;  // Colas SJF izquierda y derecha
rr_scheduler_t       rr_Izq,    rr_Der;  // Colas RR izquierda y derecha
strn_scheduler_t     strn_Izq,  strn_Der;  // Colas STRN izquierda y derecha
edf_scheduler_t      edf_Izq,   edf_Der;  // Colas EDF izquierda y derecha

// Semáforos y estado global
SemaphoreHandle_t queueMutex;  // Mutex para proteger acceso a colas
SemaphoreHandle_t doneSemaphore;  // Semáforo para indicar fin de slice
SemaphoreHandle_t sem_espacio_canal;  // Semáforo para espacio en canal
QueueHandle_t barcos_en_vuelo;  // Cola de barcos en vuelo
Barco* barcos_activos[MAX_BARCOS_CANAL];  // Array de barcos activos
int    barcos_en_canal_count = 0;  // Contador de barcos en canal
int    sentido_actual        = 0;  // Sentido actual: 0 izquierda, 1 derecha

// Función helper: verificar si hay barcos en un lado
static bool hayBarcosEnLado(int lado) {
    if (lado == 0) {  // Lado izquierdo
        switch (SCHEDULER_ACTUAL) {
            case SCHEDULER_FCFS:     return fcfs_Izq.count  > 0;
            case SCHEDULER_PRIORITY: return prio_Izq.count  > 0;
            case SCHEDULER_SJF:      return sjf_Izq.count   > 0;
            case SCHEDULER_RR:       return rr_Izq.count    > 0;
            case SCHEDULER_STRN:     return strn_Izq.count  > 0;
            case SCHEDULER_EDF:      return edf_Izq.count   > 0;
            default:                 return false;
        }
    } else {  // Lado derecho
        switch (SCHEDULER_ACTUAL) {
            case SCHEDULER_FCFS:     return fcfs_Der.count  > 0;
            case SCHEDULER_PRIORITY: return prio_Der.count  > 0;
            case SCHEDULER_SJF:      return sjf_Der.count   > 0;
            case SCHEDULER_RR:       return rr_Der.count    > 0;
            case SCHEDULER_STRN:     return strn_Der.count  > 0;
            case SCHEDULER_EDF:      return edf_Der.count   > 0;
            default:                 return false;
        }
    }
}

// Función para encolar barco
bool enqueueBarco(Barco *barco) {
    if (xSemaphoreTake(queueMutex, portMAX_DELAY) != pdTRUE) return false;  // Tomar mutex
    bool result = false;
    if (barco->lado == 0) {  // Lado izquierdo
        switch (SCHEDULER_ACTUAL) {
            case SCHEDULER_FCFS:     result = fcfs_scheduler_enqueue(&fcfs_Izq, barco);                          break;
            case SCHEDULER_PRIORITY: result = priority_scheduler_enqueue(&prio_Izq, barco, barco->prioridad);    break;
            case SCHEDULER_SJF:      result = sjf_scheduler_enqueue(&sjf_Izq, barco, barco->tiempoCruce);        break;
            case SCHEDULER_RR:       result = rr_scheduler_enqueue(&rr_Izq, barco);                              break;
            case SCHEDULER_STRN:     result = strn_scheduler_enqueue(&strn_Izq, barco, barco->tiempoRestante);   break;
            case SCHEDULER_EDF:      result = edf_scheduler_enqueue(&edf_Izq, barco, barco->deadline);           break;
        }
    } else {  // Lado derecho
        switch (SCHEDULER_ACTUAL) {
            case SCHEDULER_FCFS:     result = fcfs_scheduler_enqueue(&fcfs_Der, barco);                          break;
            case SCHEDULER_PRIORITY: result = priority_scheduler_enqueue(&prio_Der, barco, barco->prioridad);    break;
            case SCHEDULER_SJF:      result = sjf_scheduler_enqueue(&sjf_Der, barco, barco->tiempoCruce);        break;
            case SCHEDULER_RR:       result = rr_scheduler_enqueue(&rr_Der, barco);                              break;
            case SCHEDULER_STRN:     result = strn_scheduler_enqueue(&strn_Der, barco, barco->tiempoRestante);   break;
            case SCHEDULER_EDF:      result = edf_scheduler_enqueue(&edf_Der, barco, barco->deadline);           break;
        }
    }
    xSemaphoreGive(queueMutex);  // Liberar mutex
    return result;
}

// Función para desencolar barco del lado actual
Barco* dequeueBarco() {
    Barco *barco = NULL;
    if (xSemaphoreTake(queueMutex, portMAX_DELAY) != pdTRUE) return NULL;  // Tomar mutex
    if (sentido_actual == 0) {  // Sentido izquierdo
        switch (SCHEDULER_ACTUAL) {
            case SCHEDULER_FCFS:     fcfs_scheduler_dequeue(&fcfs_Izq,   (void**)&barco); break;
            case SCHEDULER_PRIORITY: priority_scheduler_dequeue(&prio_Izq,(void**)&barco); break;
            case SCHEDULER_SJF:      sjf_scheduler_dequeue(&sjf_Izq,     (void**)&barco); break;
            case SCHEDULER_RR:       rr_scheduler_dequeue(&rr_Izq,       (void**)&barco); break;
            case SCHEDULER_STRN:     strn_scheduler_dequeue(&strn_Izq,   (void**)&barco); break;
            case SCHEDULER_EDF:      edf_scheduler_dequeue(&edf_Izq,     (void**)&barco); break;
        }
    } else {  // Sentido derecho
        switch (SCHEDULER_ACTUAL) {
            case SCHEDULER_FCFS:     fcfs_scheduler_dequeue(&fcfs_Der,   (void**)&barco); break;
            case SCHEDULER_PRIORITY: priority_scheduler_dequeue(&prio_Der,(void**)&barco); break;
            case SCHEDULER_SJF:      sjf_scheduler_dequeue(&sjf_Der,     (void**)&barco); break;
            case SCHEDULER_RR:       rr_scheduler_dequeue(&rr_Der,       (void**)&barco); break;
            case SCHEDULER_STRN:     strn_scheduler_dequeue(&strn_Der,   (void**)&barco); break;
            case SCHEDULER_EDF:      edf_scheduler_dequeue(&edf_Der,     (void**)&barco); break;
        }
    }
    xSemaphoreGive(queueMutex);  // Liberar mutex
    return barco;
}

void letreroTask(void *pvParameters) {  // Tarea del letrero
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(miConfig.tiempoLetrero * 1000));  // Esperar tiempo del letrero
        // Esperar a que el canal quede vacío antes de cambiar sentido
        while (barcos_en_canal_count > 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        sentido_actual = (sentido_actual == 0) ? 1 : 0;  // Cambiar sentido
        printf("GUI:DIR,%d\n", sentido_actual);
        printf("[Letrero] Cambio de sentido -> lado %d (%s)\n",
               sentido_actual,
               sentido_actual == 0 ? "Izquierda" : "Derecha");
    }
}

void doneWatcherTask(void *pvParameters) {  // Tarea que maneja fin de slices
    Barco *terminado = NULL;
    while (true) {
        // Bloquear hasta que un barco señale fin de slice
        xSemaphoreTake(doneSemaphore, portMAX_DELAY);
        // Sacar el barco más antiguo en tránsito
        if (xQueueReceive(barcos_en_vuelo, &terminado, pdMS_TO_TICKS(100)) != pdTRUE) {
            printf("[DoneWatcher] WARN: doneSemaphore sin barco en vuelo\n");
            continue;
        }
        if ((SCHEDULER_ACTUAL == SCHEDULER_RR || SCHEDULER_ACTUAL == SCHEDULER_STRN)
             && terminado->tiempoRestante > 0) {
            // Barco no terminó: re-encolar para próximo slice
            printf("%s -> barco %d regresa a la cola | restante: %ds\n",
                   (SCHEDULER_ACTUAL == SCHEDULER_RR ? "RR" : "STRN"),
                   terminado->id, terminado->tiempoRestante);
            printf("GUI:RETURN,%d,%d\n", terminado->id, terminado->tiempoRestante);
            enqueueBarco(terminado);
        } else {
            // Barco terminó completamente
            printf("Barco %d completó el cruce\n", terminado->id);
        }
    }
}

// Función para iniciar el scheduler
void iniciarScheduler() {
    printf("GUI:SCH,%d\n",  SCHEDULER_ACTUAL);
    printf("GUI:MODO,%d\n", miConfig.modoControl);
    printf("GUI:DIR,%d\n",  sentido_actual);
    // Inicializar el canal primero
    canal_init();
    // Inicializar todos los schedulers para ambos lados
    fcfs_scheduler_init(&fcfs_Izq);     fcfs_scheduler_init(&fcfs_Der);
    priority_scheduler_init(&prio_Izq); priority_scheduler_init(&prio_Der);
    sjf_scheduler_init(&sjf_Izq);       sjf_scheduler_init(&sjf_Der);
    rr_scheduler_init(&rr_Izq);         rr_scheduler_init(&rr_Der);
    strn_scheduler_init(&strn_Izq);     strn_scheduler_init(&strn_Der);
    edf_scheduler_init(&edf_Izq);       edf_scheduler_init(&edf_Der);
    // Inicializar arreglo de barcos activos
    for (int i = 0; i < MAX_BARCOS_CANAL; i++) {
        barcos_activos[i] = NULL;
    }
    // Crear semáforos
    queueMutex        = xSemaphoreCreateMutex();
    doneSemaphore     = xSemaphoreCreateBinary();
    sem_espacio_canal = xSemaphoreCreateCounting(MAX_BARCOS_CANAL, MAX_BARCOS_CANAL);
    // Crear cola de barcos en vuelo
    barcos_en_vuelo = xQueueCreate(MAX_BARCOS_CANAL, sizeof(Barco*));
    if (!queueMutex || !doneSemaphore || !sem_espacio_canal || !barcos_en_vuelo) {
        printf("[FATAL] Error creando semáforos/colas del scheduler\n");
        return;
    }
    // Crear tarea del letrero si aplica
    if (miConfig.modoControl == MODO_LETRERO) {
        xTaskCreate(letreroTask, "Letrero", 2048, NULL, 2, NULL);
    }
    // Crear tarea doneWatcher
    xTaskCreate(doneWatcherTask, "DoneWatcher", 2048, NULL, 4, NULL);
    // Crear tarea principal del scheduler
    xTaskCreate(schedulerTask, "Scheduler", 4096, NULL, 3, NULL);
}

void schedulerTask(void *pvParameters) {  // Tarea principal del scheduler
    int barcosPasados = 0; // Para modo equidad
    while (true) {
        // Modo Tico
        if (miConfig.modoControl == MODO_TICO) {
            bool hayIzq = hayBarcosEnLado(0);
            bool hayDer = hayBarcosEnLado(1);
            if (!hayIzq && !hayDer) {
                vTaskDelay(pdMS_TO_TICKS(200));
                continue;
            }
            // Si el lado actual quedó vacío, cambiar al que tenga barcos
            if (!hayBarcosEnLado(sentido_actual)) {
                if (barcos_en_canal_count == 0) {
                    sentido_actual = (sentido_actual == 0) ? 1 : 0;
                    printf("GUI:DIR,%d\n", sentido_actual);
                    printf("[Tico] Cambio a lado %d (%s)\n",
                           sentido_actual,
                           sentido_actual == 0 ? "Izquierda" : "Derecha");
                } else {
                    vTaskDelay(pdMS_TO_TICKS(100));
                    continue;
                }
            }
            // Tomar espacio en el canal
            xSemaphoreTake(sem_espacio_canal, portMAX_DELAY);
            Barco *proximo = dequeueBarco();
            if (proximo == NULL) {
                xSemaphoreGive(sem_espacio_canal);
                continue;
            }
            // Fijar sentido si el canal estaba vacío
            if (barcos_en_canal_count == 0) {
                sentido_actual = proximo->lado;
                printf("GUI:DIR,%d\n", sentido_actual);
            }
            barcos_en_canal_count++;
            // Log del scheduler activo
            switch (SCHEDULER_ACTUAL) {
                case SCHEDULER_FCFS:     printf("FCFS -> barco %d\n", proximo->id); break;
                case SCHEDULER_PRIORITY: printf("PRIORITY -> barco %d | prio %d\n", proximo->id, proximo->prioridad); break;
                case SCHEDULER_SJF:      printf("SJF -> barco %d | tCruce %d\n", proximo->id, proximo->tiempoCruce); break;
                case SCHEDULER_RR:       printf("RR -> barco %d | restante %ds | quantum %ds\n", proximo->id, proximo->tiempoRestante, RR_QUANTUM); break;
                case SCHEDULER_STRN:     printf("STRN -> barco %d | restante %ds\n", proximo->id, proximo->tiempoRestante); break;
                case SCHEDULER_EDF:      printf("EDF -> barco %d | deadline %d\n", proximo->id, proximo->deadline); break;
            }
            int guiSlice = proximo->tiempoRestante;
            if (SCHEDULER_ACTUAL == SCHEDULER_RR && guiSlice > RR_QUANTUM) guiSlice = RR_QUANTUM;
            printf("GUI:CROSS,%d,%d,%d,%d,%d,%d\n",
                   proximo->id, proximo->tipo, proximo->lado,
                   guiSlice, proximo->tiempoCruce, proximo->tiempoRestante);
            // Registrar en cola de en vuelo antes de dar semáforo
            xQueueSend(barcos_en_vuelo, &proximo, portMAX_DELAY);
            // Despertar al barco
            xSemaphoreGive(proximo->sem);
            // Pausa para escalonar
            vTaskDelay(pdMS_TO_TICKS(300));
            continue;
        }
        // Modo Equidad / Letrero
        bool colaVacia = !hayBarcosEnLado(sentido_actual);
        if (colaVacia) {
            int otroLado = (sentido_actual == 0) ? 1 : 0;
            if (hayBarcosEnLado(otroLado) && barcos_en_canal_count == 0) {
                sentido_actual = otroLado;
                barcosPasados  = 0;
                printf("GUI:DIR,%d\n", sentido_actual);
                printf("[%s] Cola vacía, cambio a lado %d (%s)\n",
                       miConfig.modoControl == MODO_EQUIDAD ? "Equidad" : "Letrero",
                       sentido_actual,
                       sentido_actual == 0 ? "Izquierda" : "Derecha");
            }
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }
        // Modo Equidad: respetar límite W
        if (miConfig.modoControl == MODO_EQUIDAD) {
            int otroLado = (sentido_actual == 0) ? 1 : 0;
            if (barcosPasados >= miConfig.parametroW && hayBarcosEnLado(otroLado)) {
                if (barcos_en_canal_count > 0) {
                    vTaskDelay(pdMS_TO_TICKS(100));
                    continue;
                }
                sentido_actual = otroLado;
                barcosPasados  = 0;
                printf("GUI:DIR,%d\n", sentido_actual);
                printf("[Equidad] W=%d alcanzado, cambio a lado %d (%s)\n",
                       miConfig.parametroW, sentido_actual,
                       sentido_actual == 0 ? "Izquierda" : "Derecha");
                continue;
            }
        }
        // Tomar espacio en el canal
        xSemaphoreTake(sem_espacio_canal, portMAX_DELAY);
        Barco *proximo = dequeueBarco();
        if (proximo == NULL) {
            xSemaphoreGive(sem_espacio_canal);
            continue;
        }
        // Fijar sentido si el canal estaba vacío
        if (barcos_en_canal_count == 0) {
            sentido_actual = proximo->lado;
            printf("GUI:DIR,%d\n", sentido_actual);
        }
        barcos_en_canal_count++;
        switch (SCHEDULER_ACTUAL) {
            case SCHEDULER_FCFS:     printf("FCFS -> barco %d\n", proximo->id); break;
            case SCHEDULER_PRIORITY: printf("PRIORITY -> barco %d | prio %d\n", proximo->id, proximo->prioridad); break;
            case SCHEDULER_SJF:      printf("SJF -> barco %d | tCruce %d\n", proximo->id, proximo->tiempoCruce); break;
            case SCHEDULER_RR:       printf("RR -> barco %d | restante %ds | quantum %ds\n", proximo->id, proximo->tiempoRestante, RR_QUANTUM); break;
            case SCHEDULER_STRN:     printf("STRN -> barco %d | restante %ds\n", proximo->id, proximo->tiempoRestante); break;
            case SCHEDULER_EDF:      printf("EDF -> barco %d | deadline %d\n", proximo->id, proximo->deadline); break;
        }
        int guiSlice = proximo->tiempoRestante;
        if (SCHEDULER_ACTUAL == SCHEDULER_RR && guiSlice > RR_QUANTUM) guiSlice = RR_QUANTUM;
        printf("GUI:CROSS,%d,%d,%d,%d,%d,%d\n",
               proximo->id, proximo->tipo, proximo->lado,
               guiSlice, proximo->tiempoCruce, proximo->tiempoRestante);
        // Registrar en cola de en vuelo antes de despertar
        xQueueSend(barcos_en_vuelo, &proximo, portMAX_DELAY);
        // Despertar al barco
        xSemaphoreGive(proximo->sem);
        // Contar para Equidad
        if (miConfig.modoControl == MODO_EQUIDAD) {
            barcosPasados++;
        }
        // Pausa escalonada
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}
