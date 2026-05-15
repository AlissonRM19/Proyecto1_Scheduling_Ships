#ifndef RR_SCHEDULER_H  // Guardia de inclusión
#define RR_SCHEDULER_H

#include <stdbool.h>  // Incluir bool
#include <stddef.h>  // Incluir size_t
#include "config.h"  // Incluir configuración

#ifdef __cplusplus
extern "C" {  // Declarar extern C
#endif

// Reutilizamos el tamaño máximo definido para FCFS o definimos uno propio
#define RR_SCHEDULER_MAX_ITEMS 16  // Máximo items en cola RR

// Estructura de la cola RR
typedef struct {
    void *buffer[RR_SCHEDULER_MAX_ITEMS];  // Buffer de punteros
    int head;  // Cabeza
    int tail;  // Cola
    int count;  // Contador
} rr_scheduler_t;

void rr_scheduler_init(rr_scheduler_t *q);  // Inicializar cola
bool rr_scheduler_enqueue(rr_scheduler_t *q, void *item);  // Encolar item
bool rr_scheduler_dequeue(rr_scheduler_t *q, void **item);  // Desencolar item

#ifdef __cplusplus
}
#endif

#endif