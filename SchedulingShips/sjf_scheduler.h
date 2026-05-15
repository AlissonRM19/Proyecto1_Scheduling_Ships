#ifndef SJF_SCHEDULER_H  // Guardia de inclusión
#define SJF_SCHEDULER_H

#include <stdbool.h>  // Incluir bool
#include "barcos.h"  // Incluir barcos
#include "config.h"  // Incluir configuración

#ifdef __cplusplus
extern "C" {  // Declarar extern C
#endif

// Estructura del scheduler SJF
typedef struct {
    void* buffer[SJF_SCHEDULER_MAX_ITEMS];  // Buffer de punteros
    int count;  // Número de elementos
} sjf_scheduler_t;

void sjf_scheduler_init(sjf_scheduler_t *q);  // Inicializar scheduler
// Usamos void* item para que coincida con el estándar de los otros schedulers
bool sjf_scheduler_enqueue(sjf_scheduler_t *q, void *item, int tiempoCruce);  // Encolar item con tiempo de cruce
bool sjf_scheduler_dequeue(sjf_scheduler_t *q, void **item);  // Desencolar item

#ifdef __cplusplus
}
#endif

#endif
