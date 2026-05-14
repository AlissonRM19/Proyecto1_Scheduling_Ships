#include "fcfs_scheduler.h"

// Inicialización de la cola
void fcfs_scheduler_init(fcfs_scheduler_t *q) {
    if (q == NULL) return;

    // Inicializar los índices y el contador
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    
    // Limpiar el buffer para evitar basura en memoria
    for (int i = 0; i < FCFS_SCHEDULER_MAX_ITEMS; i++) {
        q->buffer[i] = NULL;
    }
}

// Enqueue: Agrega un barco al final de la cola
bool fcfs_scheduler_enqueue(fcfs_scheduler_t *q, void *item) {
    if (q == NULL || item == NULL) return false;

    // Verificar si la cola está llena
    if (q->count >= FCFS_SCHEDULER_MAX_ITEMS) {
        return false; // Cola llena
    }

    // Agregar el item al final de la cola
    //item representa un barco, pero lo tratamos como void* para mantener la generalidad
    q->buffer[q->tail] = item;
    // Aritmética circular para el índice
    q->tail = (q->tail + 1) % FCFS_SCHEDULER_MAX_ITEMS;
    q->count++;

    return true;
}

// Dequeue: Elimina y devuelve el barco al frente de la cola
bool fcfs_scheduler_dequeue(fcfs_scheduler_t *q, void **item) {
    if (q == NULL || item == NULL) return false;

    // Verificar si la cola está vacía
    if (q->count == 0) {
        *item = NULL;
        return false; // Cola vacía
    }

    // Obtener el item al frente de la cola
    *item = q->buffer[q->head];
    q->buffer[q->head] = NULL; // Limpieza preventiva
    // Aritmética circular para el índice
    q->head = (q->head + 1) % FCFS_SCHEDULER_MAX_ITEMS;
    q->count--;

    return true;
}
