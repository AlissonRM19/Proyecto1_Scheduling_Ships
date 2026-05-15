#ifndef FCFS_SCHEDULER_H  // Guardia de inclusión
#define FCFS_SCHEDULER_H

#include <stdbool.h>  // Incluir bool
#include <stddef.h>  // Incluir size_t

#ifdef __cplusplus
extern "C" {  // Declarar extern C
#endif

#define FCFS_SCHEDULER_MAX_ITEMS 16  // Máximo items en la cola

// Estructura de la cola FCFS
typedef struct {
  void *buffer[FCFS_SCHEDULER_MAX_ITEMS];  // Buffer de punteros
  int head;  // Cabeza de la cola
  int tail;  // Cola de la cola
  int count;  // Número de elementos
} fcfs_scheduler_t;

void fcfs_scheduler_init(fcfs_scheduler_t *q);  // Inicializar cola
bool fcfs_scheduler_enqueue(fcfs_scheduler_t *q, void *item);  // Encolar item
bool fcfs_scheduler_dequeue(fcfs_scheduler_t *q, void **item);  // Desencolar item

#ifdef __cplusplus
}
#endif

#endif
