#include "edf_scheduler.h"
#include "barcos.h"
#include <stddef.h>
#include <stdbool.h>

// Inicializa el scheduler
void edf_scheduler_init(edf_scheduler_t *q) {
    if (q == NULL) return;
    q->count = 0;
    // Inicializamos el buffer a NULL para evitar basura
    for (int i = 0; i < EDF_SCHEDULER_MAX_ITEMS; i++) {
        q->buffer[i] = NULL;
    }
}

// Encola un elemento con su deadline, manteniendo el orden por deadline
bool edf_scheduler_enqueue(edf_scheduler_t *q, void *item, int deadline) {
    if (q == NULL || item == NULL) return false;

    // Verificación de espacio en el scheduler
    if (q->count >= EDF_SCHEDULER_MAX_ITEMS) {
        return false;
    }

    // Casteamos para poder comparar el deadline si fuera necesario desde el struct
    // aunque aquí ya lo recibimos por parámetro.
    int i = q->count;

    // Inserción ordenada: Menor deadline (más urgente) al inicio
    while (i > 0) {
        // Accedemos al barco anterior para comparar su deadline
        Barco *anterior = (Barco *)q->buffer[i - 1];

        if (deadline < anterior->deadline) {
            q->buffer[i] = q->buffer[i - 1];
            i--;
        } else {
            break;
        }
    }

    // Insertamos el nuevo elemento en la posición correcta
    q->buffer[i] = item;
    q->count++;
    return true;
}

// Desencola el elemento más urgente (con menor deadline)
bool edf_scheduler_dequeue(edf_scheduler_t *q, void **item) {
    if (q == NULL || item == NULL || q->count == 0) {
        return false;
    }

    // El más urgente siempre está en la posición 0
    *item = q->buffer[0];

    // Desplazar a la izquierda (Shift left)
    for (int i = 0; i < q->count - 1; i++) {
        q->buffer[i] = q->buffer[i + 1];
    }

    q->count--;
    q->buffer[q->count] = NULL; // Limpiar la última posición
    return true;
}