#ifndef STRN_SCHEDULER_H  // Guardia de inclusión
#define STRN_SCHEDULER_H

#include <stdbool.h>  // Incluir bool
#include <stddef.h>  // Incluir size_t
#include "barcos.h"  // Incluir barcos
#include "config.h"  // Incluir configuración para STRN_SCHEDULER_MAX_ITEMS

#ifdef __cplusplus
extern "C" {  // Declarar extern C
#endif

// Estructura del scheduler STRN
typedef struct {
    void* buffer[STRN_SCHEDULER_MAX_ITEMS];  // Buffer de punteros
    int count;  // Número de elementos
} strn_scheduler_t;

void strn_scheduler_init(strn_scheduler_t *q);  // Inicializar scheduler
bool strn_scheduler_enqueue(strn_scheduler_t *q, void *item, int tiempo);  // Encolar item con tiempo
bool strn_scheduler_dequeue(strn_scheduler_t *q, void **item);  // Desencolar item

#ifdef __cplusplus
}
#endif

#endif
