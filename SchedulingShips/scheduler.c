/** 
 * Calendarizador de barcos con soporte de pipeline:
 * múltiples barcos del mismo lado pueden cruzar el canal simultáneamente,
 * controlados por sem_espacio_canal (counting semaphore = MAX_BARCOS_CANAL).
 */

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "scheduler.h"
#include "barcos.h"
#include "canal.h"
#include "fcfs_scheduler.h"
#include "priority_scheduler.h"
#include "sjf_scheduler.h"
#include "rr_scheduler.h"
#include "strn_scheduler.h"
#include "edf_scheduler.h"
#include "config.h"

// ===== COLAS DE SCHEDULERS (una por lado) =====
fcfs_scheduler_t     fcfs_Izq,  fcfs_Der;
priority_scheduler_t prio_Izq,  prio_Der;
sjf_scheduler_t      sjf_Izq,   sjf_Der;
rr_scheduler_t       rr_Izq,    rr_Der;
strn_scheduler_t     strn_Izq,  strn_Der;
edf_scheduler_t      edf_Izq,   edf_Der;

// ===== SEMÁFOROS Y ESTADO GLOBAL =====
SemaphoreHandle_t queueMutex;       // Protege acceso a las colas de schedulers
SemaphoreHandle_t doneSemaphore;    // Cada barco da un Give al terminar su slice
SemaphoreHandle_t sem_espacio_canal;// Counting semaphore: limita barcos simultáneos en canal

QueueHandle_t barcos_en_vuelo;

Barco* barcos_activos[MAX_BARCOS_CANAL];
int    barcos_en_canal_count = 0;
int    sentido_actual        = 0;   // 0 = Izquierda, 1 = Derecha

// ===== HELPER: ¿hay barcos en un lado? =====
static bool hayBarcosEnLado(int lado) {
    if (lado == 0) {
        switch (SCHEDULER_ACTUAL) {
            case SCHEDULER_FCFS:     return fcfs_Izq.count  > 0;
            case SCHEDULER_PRIORITY: return prio_Izq.count  > 0;
            case SCHEDULER_SJF:      return sjf_Izq.count   > 0;
            case SCHEDULER_RR:       return rr_Izq.count    > 0;
            case SCHEDULER_STRN:     return strn_Izq.count  > 0;
            case SCHEDULER_EDF:      return edf_Izq.count   > 0;
            default:                 return false;
        }
    } else {
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

// ===== ENQUEUE =====
bool enqueueBarco(Barco *barco) {
    if (xSemaphoreTake(queueMutex, portMAX_DELAY) != pdTRUE) return false;
    bool result = false;
    if (barco->lado == 0) {
        switch (SCHEDULER_ACTUAL) {
            case SCHEDULER_FCFS:     result = fcfs_scheduler_enqueue(&fcfs_Izq, barco);                          break;
            case SCHEDULER_PRIORITY: result = priority_scheduler_enqueue(&prio_Izq, barco, barco->prioridad);    break;
            case SCHEDULER_SJF:      result = sjf_scheduler_enqueue(&sjf_Izq, barco, barco->tiempoCruce);        break;
            case SCHEDULER_RR:       result = rr_scheduler_enqueue(&rr_Izq, barco);                              break;
            case SCHEDULER_STRN:     result = strn_scheduler_enqueue(&strn_Izq, barco, barco->tiempoRestante);   break;
            case SCHEDULER_EDF:      result = edf_scheduler_enqueue(&edf_Izq, barco, barco->deadline);           break;
        }
    } else {
        switch (SCHEDULER_ACTUAL) {
            case SCHEDULER_FCFS:     result = fcfs_scheduler_enqueue(&fcfs_Der, barco);                          break;
            case SCHEDULER_PRIORITY: result = priority_scheduler_enqueue(&prio_Der, barco, barco->prioridad);    break;
            case SCHEDULER_SJF:      result = sjf_scheduler_enqueue(&sjf_Der, barco, barco->tiempoCruce);        break;
            case SCHEDULER_RR:       result = rr_scheduler_enqueue(&rr_Der, barco);                              break;
            case SCHEDULER_STRN:     result = strn_scheduler_enqueue(&strn_Der, barco, barco->tiempoRestante);   break;
            case SCHEDULER_EDF:      result = edf_scheduler_enqueue(&edf_Der, barco, barco->deadline);           break;
        }
    }
    xSemaphoreGive(queueMutex);
    return result;
}

// ===== DEQUEUE (del lado sentido_actual) =====
Barco* dequeueBarco() {
    Barco *barco = NULL;
    if (xSemaphoreTake(queueMutex, portMAX_DELAY) != pdTRUE) return NULL;
    if (sentido_actual == 0) {
        switch (SCHEDULER_ACTUAL) {
            case SCHEDULER_FCFS:     fcfs_scheduler_dequeue(&fcfs_Izq,   (void**)&barco); break;
            case SCHEDULER_PRIORITY: priority_scheduler_dequeue(&prio_Izq,(void**)&barco); break;
            case SCHEDULER_SJF:      sjf_scheduler_dequeue(&sjf_Izq,     (void**)&barco); break;
            case SCHEDULER_RR:       rr_scheduler_dequeue(&rr_Izq,       (void**)&barco); break;
            case SCHEDULER_STRN:     strn_scheduler_dequeue(&strn_Izq,   (void**)&barco); break;
            case SCHEDULER_EDF:      edf_scheduler_dequeue(&edf_Izq,     (void**)&barco); break;
        }
    } else {
        switch (SCHEDULER_ACTUAL) {
            case SCHEDULER_FCFS:     fcfs_scheduler_dequeue(&fcfs_Der,   (void**)&barco); break;
            case SCHEDULER_PRIORITY: priority_scheduler_dequeue(&prio_Der,(void**)&barco); break;
            case SCHEDULER_SJF:      sjf_scheduler_dequeue(&sjf_Der,     (void**)&barco); break;
            case SCHEDULER_RR:       rr_scheduler_dequeue(&rr_Der,       (void**)&barco); break;
            case SCHEDULER_STRN:     strn_scheduler_dequeue(&strn_Der,   (void**)&barco); break;
            case SCHEDULER_EDF:      edf_scheduler_dequeue(&edf_Der,     (void**)&barco); break;
        }
    }
    xSemaphoreGive(queueMutex);
    return barco;
}


void letreroTask(void *pvParameters) {
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(miConfig.tiempoLetrero * 1000));

        // Esperar a que el canal quede vacío antes de cambiar sentido
        while (barcos_en_canal_count > 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        sentido_actual = (sentido_actual == 0) ? 1 : 0;
        printf("GUI:DIR,%d\n", sentido_actual);
        printf("[Letrero] Cambio de sentido -> lado %d (%s)\n",
               sentido_actual,
               sentido_actual == 0 ? "Izquierda" : "Derecha");
    }
}

void doneWatcherTask(void *pvParameters) {
    Barco *terminado = NULL;

    while (true) {
        // Bloquear hasta que cualquier barco señale fin de su slice
        xSemaphoreTake(doneSemaphore, portMAX_DELAY);

        // Sacar el barco más antiguo en tránsito (orden FIFO garantiza que
        // el primero en entrar es el primero en reportar done)
        if (xQueueReceive(barcos_en_vuelo, &terminado, pdMS_TO_TICKS(100)) != pdTRUE) {
            // No debería pasar, pero protegemos por robustez
            printf("[DoneWatcher] WARN: doneSemaphore sin barco en vuelo\n");
            continue;
        }

        if ((SCHEDULER_ACTUAL == SCHEDULER_RR || SCHEDULER_ACTUAL == SCHEDULER_STRN)
             && terminado->tiempoRestante > 0) {
            // Barco no terminó: re-encolar para el próximo slice
            printf("%s -> barco %d regresa a la cola | restante: %ds\n",
                   (SCHEDULER_ACTUAL == SCHEDULER_RR ? "RR" : "STRN"),
                   terminado->id, terminado->tiempoRestante);
            printf("GUI:RETURN,%d,%d\n", terminado->id, terminado->tiempoRestante);
            enqueueBarco(terminado);
        } else {
            // Barco terminó completamente (barcoTask hará free y vTaskDelete)
            printf("Barco %d completó el cruce\n", terminado->id);
            // GUI:DONE lo emite barcoTask al salir del while de cruce
        }
    }
}

// ===== INIT =====
void iniciarScheduler() {
    printf("GUI:SCH,%d\n",  SCHEDULER_ACTUAL);
    printf("GUI:MODO,%d\n", miConfig.modoControl);
    printf("GUI:DIR,%d\n",  sentido_actual);

    // Inicializar el canal (mutexes de posiciones) PRIMERO
    // Ningún barco puede entrar antes de que esto esté listo
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

    // Semáforos
    queueMutex        = xSemaphoreCreateMutex();
    doneSemaphore     = xSemaphoreCreateBinary();
    sem_espacio_canal = xSemaphoreCreateCounting(MAX_BARCOS_CANAL, MAX_BARCOS_CANAL);

    // Cola de barcos en vuelo (tamaño = máx simultáneos en canal)
    barcos_en_vuelo = xQueueCreate(MAX_BARCOS_CANAL, sizeof(Barco*));

    if (!queueMutex || !doneSemaphore || !sem_espacio_canal || !barcos_en_vuelo) {
        printf("[FATAL] Error creando semáforos/colas del scheduler\n");
        return;
    }

    // Tarea del letrero (solo si aplica)
    if (miConfig.modoControl == MODO_LETRERO) {
        xTaskCreate(letreroTask, "Letrero", 2048, NULL, 2, NULL);
    }

    // Tarea que consume doneSemaphore y maneja re-encolas RR/STRN
    xTaskCreate(doneWatcherTask, "DoneWatcher", 2048, NULL, 4, NULL);

    // Tarea principal del scheduler (despacha barcos)
    xTaskCreate(schedulerTask, "Scheduler", 4096, NULL, 3, NULL);
}

void schedulerTask(void *pvParameters) {

    int barcosPasados = 0; // Para MODO_EQUIDAD

    while (true) {

        // =====================================================
        // MODO TICO
        // =====================================================
        if (miConfig.modoControl == MODO_TICO) {
            bool hayIzq = hayBarcosEnLado(0);
            bool hayDer = hayBarcosEnLado(1);

            if (!hayIzq && !hayDer) {
                vTaskDelay(pdMS_TO_TICKS(200));
                continue;
            }

            // Si el lado actual quedó vacío, cambiar al que tenga barcos
            if (!hayBarcosEnLado(sentido_actual)) {
                // Solo cambiar si el canal está libre (evitar colisión)
                if (barcos_en_canal_count == 0) {
                    sentido_actual = (sentido_actual == 0) ? 1 : 0;
                    printf("GUI:DIR,%d\n", sentido_actual);
                    printf("[Tico] Cambio a lado %d (%s)\n",
                           sentido_actual,
                           sentido_actual == 0 ? "Izquierda" : "Derecha");
                } else {
                    // Canal ocupado con dirección contraria, esperar
                    vTaskDelay(pdMS_TO_TICKS(100));
                    continue;
                }
            }

            // Tomar espacio en el canal (bloquea si está lleno)
            xSemaphoreTake(sem_espacio_canal, portMAX_DELAY);

            Barco *proximo = dequeueBarco();
            if (proximo == NULL) {
                // La cola se vació entre el check y el dequeue
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

            // Registrar en cola de en vuelo ANTES de dar el semáforo al barco
            xQueueSend(barcos_en_vuelo, &proximo, portMAX_DELAY);

            // Despertar al barco para que cruce — scheduler NO bloquea aquí
            xSemaphoreGive(proximo->sem);

            // Pequeña pausa para que el siguiente barco entre escalonado
            // (ajustar según velocidad deseada del pipeline)
            vTaskDelay(pdMS_TO_TICKS(300));
            continue;
        }

        // =====================================================
        // MODO EQUIDAD / LETRERO
        // =====================================================
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

        // MODO EQUIDAD: respetar límite W
        if (miConfig.modoControl == MODO_EQUIDAD) {
            int otroLado = (sentido_actual == 0) ? 1 : 0;
            if (barcosPasados >= miConfig.parametroW && hayBarcosEnLado(otroLado)) {
                // Esperar que el canal se vacíe antes de cambiar sentido
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

        // Registrar en cola de en vuelo ANTES de despertar al barco
        xQueueSend(barcos_en_vuelo, &proximo, portMAX_DELAY);

        // Despertar al barco — scheduler NO espera que termine
        xSemaphoreGive(proximo->sem);

        // Contar para Equidad (se cuenta al despachar, no al terminar)
        if (miConfig.modoControl == MODO_EQUIDAD) {
            barcosPasados++;
        }

        // Pausa escalonada entre despachos consecutivos del mismo lado
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}
