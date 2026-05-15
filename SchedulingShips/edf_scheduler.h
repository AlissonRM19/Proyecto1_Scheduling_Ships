#ifndef EDF_SCHEDULER_H  // Guardia de inclusión
#define EDF_SCHEDULER_H

#include <stdbool.h>  // Incluir bool
#include <stddef.h>  // Incluir size_t

#ifdef __cplusplus
extern "C" {  // Declarar extern C
#endif

#define EDF_SCHEDULER_MAX_ITEMS 16  // Máximo items en EDF

// Estructura del scheduler EDF
typedef struct {
    void* buffer[EDF_SCHEDULER_MAX_ITEMS];  // Buffer de punteros
    int count;  // Número de elementos
} edf_scheduler_t;

void edf_scheduler_init(edf_scheduler_t* q);  // Inicializar scheduler
bool edf_scheduler_enqueue(edf_scheduler_t* q, void* item, int deadline);  // Encolar item con deadline
bool edf_scheduler_dequeue(edf_scheduler_t* q, void** item);  // Desencolar item

#ifdef __cplusplus
}
#endif

#endif