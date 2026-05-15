#include "edf_scheduler.h"  // Incluir EDF scheduler
#include "barcos.h"  // Incluir barcos
#include <stddef.h>  // Incluir size_t
#include <stdbool.h>  // Incluir bool

// Inicializa el scheduler
void edf_scheduler_init(edf_scheduler_t *q) {
    if (q == NULL) return;  // Verificar puntero nulo
    q->count = 0;  // Contador a 0
    // Inicializamos el buffer a NULL para evitar basura
    for (int i = 0; i < EDF_SCHEDULER_MAX_ITEMS; i++) {
        q->buffer[i] = NULL;  // Poner NULL
    }
}

// Encola un elemento con su deadline, manteniendo el orden por deadline
bool edf_scheduler_enqueue(edf_scheduler_t *q, void *item, int deadline) {
    if (q == NULL || item == NULL) return false;  // Verificar punteros

    // Verificación de espacio en el scheduler
    if (q->count >= EDF_SCHEDULER_MAX_ITEMS) {
        return false;  // Cola llena
    }

    // Casteamos para poder comparar el deadline si fuera necesario desde el struct
    // aunque aquí ya lo recibimos por parámetro.
    int i = q->count;  // Índice de inserción

    // Inserción ordenada: Menor deadline (más urgente) al inicio
    while (i > 0) {
        // Accedemos al barco anterior para comparar su deadline
        Barco *anterior = (Barco *)q->buffer[i - 1];  // Cast a Barco

        if (deadline < anterior->deadline) {
            q->buffer[i] = q->buffer[i - 1];  // Mover elemento
            i--;  // Decrementar índice
        } else {
            break;  // Salir
        }
    }

    // Insertamos el nuevo elemento en la posición correcta
    q->buffer[i] = item;  // Insertar item
    q->count++;  // Incrementar contador
    return true;  // Éxito
}

// Desencola el elemento más urgente (con menor deadline)
bool edf_scheduler_dequeue(edf_scheduler_t *q, void **item) {
    if (q == NULL || item == NULL || q->count == 0) {
        return false;  // Verificar punteros y si vacía
    }

    // El más urgente siempre está en la posición 0
    *item = q->buffer[0];  // Obtener primer item

    // Desplazar a la izquierda (Shift left)
    for (int i = 0; i < q->count - 1; i++) {
        q->buffer[i] = q->buffer[i + 1];  // Mover elementos
    }

    q->count--;  // Decrementar contador
    q->buffer[q->count] = NULL; // Limpiar la última posición  // Limpiar posición
    return true;  // Éxito
}