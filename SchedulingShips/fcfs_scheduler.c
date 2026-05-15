#include "fcfs_scheduler.h"  // Incluir cabecera FCFS

// Inicialización de la cola
void fcfs_scheduler_init(fcfs_scheduler_t *q) {
    if (q == NULL) return;  // Verificar puntero nulo

    // Inicializar los índices y el contador
    q->head = 0;  // Cabeza a 0
    q->tail = 0;  // Cola a 0
    q->count = 0;  // Contador a 0
    
    // Limpiar el buffer para evitar basura en memoria
    for (int i = 0; i < FCFS_SCHEDULER_MAX_ITEMS; i++) {
        q->buffer[i] = NULL;  // Poner NULL en cada posición
    }
}

// Enqueue: Agrega un barco al final de la cola
bool fcfs_scheduler_enqueue(fcfs_scheduler_t *q, void *item) {
    if (q == NULL || item == NULL) return false;  // Verificar punteros

    // Verificar si la cola está llena
    if (q->count >= FCFS_SCHEDULER_MAX_ITEMS) {
        return false; // Cola llena
    }

    // Agregar el item al final de la cola
    q->buffer[q->tail] = item;  // Asignar item
    // Aritmética circular para el índice
    q->tail = (q->tail + 1) % FCFS_SCHEDULER_MAX_ITEMS;  // Incrementar tail circular
    q->count++;  // Incrementar contador

    return true;  // Éxito
}

// Dequeue: Elimina y devuelve el barco al frente de la cola
bool fcfs_scheduler_dequeue(fcfs_scheduler_t *q, void **item) {
    if (q == NULL || item == NULL) return false;  // Verificar punteros

    // Verificar si la cola está vacía
    if (q->count == 0) {
        *item = NULL;  // Poner NULL
        return false; // Cola vacía
    }

    // Obtener el item al frente de la cola
    *item = q->buffer[q->head];  // Obtener item
    // Aritmética circular para el índice
    q->head = (q->head + 1) % FCFS_SCHEDULER_MAX_ITEMS;  // Incrementar head circular
    q->count--;  // Decrementar contador

    return true;  // Éxito
}