#include <stddef.h>  // Incluir size_t
#include <stdbool.h>  // Incluir bool
#include "sjf_scheduler.h"  // Incluir SJF scheduler
#include "barcos.h"  // Incluir barcos

// Inicializa la cola de Shortest Job First
void sjf_scheduler_init(sjf_scheduler_t* q) {
    if (q == NULL) return;  // Verificar puntero nulo
    // Inicializamos el contador y el buffer de la cola
    q->count = 0;  // Contador a 0
    // Inicializamos el buffer con NULL para evitar basura
    for (int i = 0; i < SJF_SCHEDULER_MAX_ITEMS; i++) {
        q->buffer[i] = NULL;  // Poner NULL
    }
}

// Cambiado Barco* a void* para coincidir con el header
bool sjf_scheduler_enqueue(sjf_scheduler_t* q, void* item, int tiempoCruce) {
    if (q == NULL || item == NULL) return false;  // Verificar punteros
    if (q->count >= SJF_SCHEDULER_MAX_ITEMS) return false;  // Verificar si llena

    int i = q->count;  // Índice de inserción

    // Aquí hacemos el cast para poder leer .tiempoCruce del barco que se va a encolar
    while (i > 0) {
        // Aquí hacemos el cast para poder leer .tiempoCruce
        Barco* anterior = (Barco*)q->buffer[i - 1];  // Cast a Barco
        
        // Comparamos los tiempos de cruce para mantener la cola ordenada de menor a mayor
        if (tiempoCruce < anterior->tiempoCruce) {
            q->buffer[i] = q->buffer[i - 1];  // Mover elemento
            i--;  // Decrementar índice
        } else {
            break;  // Salir del bucle
        }
    }

    // Insertamos el nuevo barco en la posición correcta
    q->buffer[i] = item;  // Insertar item
    q->count++;  // Incrementar contador
    return true;  // Éxito
}

// Cambiado Barco** a void** para coincidir con el header
bool sjf_scheduler_dequeue(sjf_scheduler_t* q, void** item) {
    if (q == NULL || item == NULL) return false;  // Verificar punteros
    if (q->count == 0) return false;  // Verificar si vacía

    *item = q->buffer[0];  // Obtener primer item

    for (int i = 0; i < q->count - 1; i++) {
        q->buffer[i] = q->buffer[i + 1];  // Mover elementos
    }

    // Limpiamos la última posición y decrementamos el contador
    q->count--;  // Decrementar contador
    return true;  // Éxito
}
