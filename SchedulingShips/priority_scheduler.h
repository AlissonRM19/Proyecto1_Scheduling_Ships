#ifndef PRIORITY_SCHEDULER_H  // Guardia de inclusión
#define PRIORITY_SCHEDULER_H

#include <stdbool.h>  // Incluir bool
#include "barcos.h"  // Incluir barcos

#ifdef __cplusplus
extern "C" {  // Declarar extern C
#endif

#define MAX_BARCOS_COLA 4  // Máximo barcos en cola

// Estructura del scheduler de prioridad
typedef struct {
  void* buffer[MAX_BARCOS_COLA];  // Buffer de punteros
  int count;  // Número de elementos
} priority_scheduler_t;

void priority_scheduler_init(priority_scheduler_t *s);  // Inicializar scheduler
bool priority_scheduler_enqueue(priority_scheduler_t *s, Barco *b, int p);  // Encolar barco con prioridad
bool priority_scheduler_dequeue(priority_scheduler_t *s, void **b);  // Desencolar barco

#ifdef __cplusplus
}
#endif

#endif
